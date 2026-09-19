// ============================================================================
// persistence.cpp — serialization (v5, fail-closed), memory store, and the
// gated service with orthogonal identities, immutable materializations and
// policy-as-data verdicts (v3 phase 2C)
// ============================================================================

#include <qiven/context/persistence.hpp>

#include <qiven/context/runtime.hpp> // IsContinueable inspects participant fields

#include <qiven/contracts.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace qiven::context
{
namespace
{
constexpr std::uint8_t kSerializationVersion = 5;
constexpr std::uint32_t kMaxRecords          = 100000; // resource-abuse guard (DR-009)

// --- little-endian TLV writers (fixed field order, versioned) ----------------

void putU8(Bytes& bytes, std::uint8_t value)
{
    bytes.push_back(std::byte { value });
}

void putU32(Bytes& bytes, std::uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8)
    {
        const auto byte = static_cast<unsigned char>((value >> shift) & 0xFFU);
        bytes.push_back(std::byte { byte });
    }
}

void putU64(Bytes& bytes, std::uint64_t value)
{
    for (unsigned shift = 0; shift < 64; shift += 8)
    {
        const auto byte = static_cast<unsigned char>((value >> shift) & 0xFFU);
        bytes.push_back(std::byte { byte });
    }
}

void putI64(Bytes& bytes, std::int64_t value)
{
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    putU64(bytes, bits);
}

void putStr(Bytes& bytes, const std::string& value)
{
    putU32(bytes, static_cast<std::uint32_t>(value.size()));
    for (const char ch : value)
    {
        bytes.push_back(std::byte { static_cast<unsigned char>(ch) });
    }
}

// --- checked reader: every input-path failure is a typed error, never an
// assert (DR-009 — asserts compile away in Release and the artifact path is
// untrusted input) -----------------------------------------------------------

struct Reader
{
    const Bytes& bytes;
    std::size_t offset { 0 };
    std::optional<DeserializeError> error;

    [[nodiscard]] bool ok() const
    {
        return !error.has_value();
    }

    [[nodiscard]] bool need(std::size_t count, const char* what)
    {
        if (ok() && offset + count > bytes.size())
        {
            error = DeserializeError { DeserializeError::Kind::Truncated, offset, what };
            return false;
        }
        return ok();
    }

    [[nodiscard]] std::uint8_t u8(const char* what)
    {
        if (!need(1, what))
        {
            return 0;
        }
        return std::to_integer<std::uint8_t>(bytes[offset++]);
    }

    [[nodiscard]] std::uint32_t u32(const char* what)
    {
        if (!need(4, what))
        {
            return 0;
        }
        std::uint32_t value = 0;
        for (unsigned shift = 0; shift < 32; shift += 8)
        {
            value |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset++])) << shift;
        }
        return value;
    }

    [[nodiscard]] std::uint64_t u64(const char* what)
    {
        if (!need(8, what))
        {
            return 0;
        }
        std::uint64_t value = 0;
        for (unsigned shift = 0; shift < 64; shift += 8)
        {
            value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset++])) << shift;
        }
        return value;
    }

    [[nodiscard]] std::int64_t i64(const char* what)
    {
        const std::uint64_t bits = u64(what);
        std::int64_t value       = 0;
        std::memcpy(&value, &bits, sizeof value);
        return value;
    }

    [[nodiscard]] std::string str(const char* what)
    {
        const auto length = u32(what);
        if (!ok())
        {
            return {};
        }
        if (!need(length, "string length exceeds input"))
        {
            return {};
        }
        std::string value;
        value.resize(length);
        if (length > 0)
        {
            std::memcpy(value.data(), bytes.data() + offset, length);
        }
        offset += length;
        return value;
    }

    template <typename E>
    [[nodiscard]] E enumValue(const char* what, std::uint8_t maxValue)
    {
        const auto raw = u8(what);
        if (!ok())
        {
            return static_cast<E>(0);
        }
        if (raw > maxValue)
        {
            error = DeserializeError { DeserializeError::Kind::BadEnum, offset - 1, what };
            return static_cast<E>(0);
        }
        return static_cast<E>(raw);
    }

    [[nodiscard]] std::uint32_t cappedCount(const char* what)
    {
        const auto count = u32(what);
        if (ok() && count > kMaxRecords)
        {
            error = DeserializeError { DeserializeError::Kind::ResourceAbuse, offset - 4, what };
            return 0;
        }
        return count;
    }

    std::vector<std::string> stringVector(const char* what)
    {
        const auto count = cappedCount(what);
        std::vector<std::string> values;
        values.reserve(count);
        for (std::uint32_t i = 0; ok() && i < count; ++i)
        {
            values.push_back(str(what));
        }
        return values;
    }
};

Bytes serializeImpl(const Snapshot& snapshot)
{
    Bytes bytes;
    putU8(bytes, kSerializationVersion);

    putStr(bytes, snapshot.governance.rootPrincipal);

    putU32(bytes, static_cast<std::uint32_t>(snapshot.constitution.articles.size()));
    for (const auto& article : snapshot.constitution.articles)
    {
        putStr(bytes, article);
    }

    putU32(bytes, static_cast<std::uint32_t>(snapshot.policy.handoff.size()));
    for (const auto& row : snapshot.policy.handoff)
    {
        putU8(bytes, static_cast<std::uint8_t>(row.opClass));
        putU8(bytes, row.requiresH2 ? 1 : 0);
        putU8(bytes, row.rootPrincipalOnly ? 1 : 0);
    }
    putU32(bytes, static_cast<std::uint32_t>(snapshot.policy.recovery.size()));
    for (const auto& row : snapshot.policy.recovery)
    {
        putU8(bytes, static_cast<std::uint8_t>(row.reason));
        putU8(bytes, static_cast<std::uint8_t>(row.action));
    }

    putStr(bytes, snapshot.state.objective);
    putStr(bytes, snapshot.state.current);
    putStr(bytes, snapshot.state.checkpointRef);
    putStr(bytes, snapshot.state.candidateRef);
    putStr(bytes, snapshot.state.nextBoundaryRef);
    putStr(bytes, snapshot.state.repositories);
    putStr(bytes, snapshot.state.roadmap);

    putU32(bytes, static_cast<std::uint32_t>(snapshot.decisions.size()));
    for (const auto& decision : snapshot.decisions)
    {
        putI64(bytes, decision.id);
        putU8(bytes, static_cast<std::uint8_t>(decision.status));
        putStr(bytes, decision.title);
        putStr(bytes, decision.content);
        putU32(bytes, static_cast<std::uint32_t>(decision.supersedes.size()));
        for (const auto id : decision.supersedes)
        {
            putI64(bytes, id);
        }
        putU32(bytes, static_cast<std::uint32_t>(decision.supersededBy.size()));
        for (const auto id : decision.supersededBy)
        {
            putI64(bytes, id);
        }
        putU32(bytes, static_cast<std::uint32_t>(decision.provenance.sources.size()));
        for (const auto& source : decision.provenance.sources)
        {
            putStr(bytes, source);
        }
    }

    putU32(bytes, static_cast<std::uint32_t>(snapshot.memory.size()));
    for (const auto& record : snapshot.memory)
    {
        putU8(bytes, static_cast<std::uint8_t>(record.kind));
        putU8(bytes, static_cast<std::uint8_t>(record.status));
        putStr(bytes, record.title);
        putStr(bytes, record.statement);
        putU32(bytes, static_cast<std::uint32_t>(record.provenance.sources.size()));
        for (const auto& source : record.provenance.sources)
        {
            putStr(bytes, source);
        }
    }

    putU32(bytes, static_cast<std::uint32_t>(snapshot.obligations.size()));
    for (const auto& obligation : snapshot.obligations)
    {
        putU8(bytes, static_cast<std::uint8_t>(obligation.status));
        putU8(bytes, static_cast<std::uint8_t>(obligation.trigger));
        putI64(bytes, obligation.id);
        putStr(bytes, obligation.statement);
        putStr(bytes, obligation.completionCriteria);
    }

    putU32(bytes, static_cast<std::uint32_t>(snapshot.evidence.size()));
    for (const auto& evidence : snapshot.evidence)
    {
        putStr(bytes, evidence.boundTo);
        putStr(bytes, evidence.content);
    }

    putU32(bytes, static_cast<std::uint32_t>(snapshot.conflicts.size()));
    for (const auto& conflict : snapshot.conflicts)
    {
        putStr(bytes, conflict.id);
        putU8(bytes, static_cast<std::uint8_t>(conflict.status));
        putStr(bytes, conflict.scope);
        putStr(bytes, conflict.description);
        putStr(bytes, conflict.resolution);
    }

    putU32(bytes, static_cast<std::uint32_t>(snapshot.profiles.size()));
    for (const auto& profile : snapshot.profiles)
    {
        putStr(bytes, profile.id);
        putStr(bytes, profile.kind);
        putStr(bytes, profile.summary);
    }

    putU32(bytes, static_cast<std::uint32_t>(snapshot.views.size()));
    for (const auto& view : snapshot.views)
    {
        putStr(bytes, view.id);
        putStr(bytes, view.agent);
        putStr(bytes, view.human);
        putStr(bytes, view.summary);
        putU32(bytes, static_cast<std::uint32_t>(view.profileRefs.size()));
        for (const auto& ref : view.profileRefs)
        {
            putStr(bytes, ref);
        }
    }

    return bytes;
}

DeserializeResult deserializeImpl(const Bytes& bytes)
{
    DeserializeResult result;
    Reader reader { bytes };

    const auto version = reader.u8("serialization version");
    if (reader.ok() && version != kSerializationVersion)
    {
        reader.error = DeserializeError { DeserializeError::Kind::BadVersion, 0, "version" };
    }

    Snapshot& snapshot = result.snapshot;
    if (reader.ok())
    {
        snapshot.governance.rootPrincipal = reader.str("root principal");
        snapshot.constitution.articles    = reader.stringVector("constitution articles");

        const auto handoffCount = reader.cappedCount("handoff policy rows");
        snapshot.policy.handoff.reserve(handoffCount);
        for (std::uint32_t i = 0; reader.ok() && i < handoffCount; ++i)
        {
            HandoffPolicy row;
            row.opClass           = reader.enumValue<OperationClass>("handoff op class", 5);
            row.requiresH2        = reader.u8("handoff requiresH2") != 0;
            row.rootPrincipalOnly = reader.u8("handoff rootPrincipalOnly") != 0;
            snapshot.policy.handoff.push_back(row);
        }
        const auto recoveryCount = reader.cappedCount("recovery rows");
        snapshot.policy.recovery.reserve(recoveryCount);
        for (std::uint32_t i = 0; reader.ok() && i < recoveryCount; ++i)
        {
            RecoveryRule row;
            row.reason = reader.enumValue<RefusalReason>("recovery reason", 11);
            row.action = reader.enumValue<RecoveryAction>("recovery action", 5);
            snapshot.policy.recovery.push_back(row);
        }

        if (reader.ok())
        {
            snapshot.state.objective       = reader.str("state.objective");
            snapshot.state.current         = reader.str("state.current");
            snapshot.state.checkpointRef   = reader.str("state.checkpointRef");
            snapshot.state.candidateRef    = reader.str("state.candidateRef");
            snapshot.state.nextBoundaryRef = reader.str("state.nextBoundaryRef");
            snapshot.state.repositories    = reader.str("state.repositories");
            snapshot.state.roadmap         = reader.str("state.roadmap");
        }
    }

    if (reader.ok())
    {
        const auto decisionCount = reader.cappedCount("decision count");
        snapshot.decisions.reserve(decisionCount);
        for (std::uint32_t i = 0; reader.ok() && i < decisionCount; ++i)
        {
            Decision decision;
            decision.id     = reader.i64("decision id");
            decision.status = reader.enumValue<Lifecycle>("decision status", 3);
            if (!reader.ok())
            {
                break;
            }
            decision.title             = reader.str("decision title");
            decision.content           = reader.str("decision content");
            const auto supersedesCount = reader.cappedCount("decision supersedes");
            decision.supersedes.reserve(supersedesCount);
            for (std::uint32_t k = 0; reader.ok() && k < supersedesCount; ++k)
            {
                decision.supersedes.push_back(reader.i64("superseded id"));
            }
            const auto supersededByCount = reader.cappedCount("decision supersededBy");
            decision.supersededBy.reserve(supersededByCount);
            for (std::uint32_t k = 0; reader.ok() && k < supersededByCount; ++k)
            {
                decision.supersededBy.push_back(reader.i64("supersededBy id"));
            }
            decision.provenance.sources = reader.stringVector("decision provenance");
            if (reader.ok())
            {
                snapshot.decisions.push_back(std::move(decision));
            }
        }
    }

    if (reader.ok())
    {
        const auto memoryCount = reader.cappedCount("memory count");
        snapshot.memory.reserve(memoryCount);
        for (std::uint32_t i = 0; reader.ok() && i < memoryCount; ++i)
        {
            MemoryRecord record;
            record.kind   = reader.enumValue<MemoryRecord::Kind>("memory kind", 5);
            record.status = reader.enumValue<MemoryRecord::Status>("memory status", 1);
            if (!reader.ok())
            {
                break;
            }
            record.title              = reader.str("memory title");
            record.statement          = reader.str("memory statement");
            record.provenance.sources = reader.stringVector("memory provenance");
            if (reader.ok())
            {
                snapshot.memory.push_back(std::move(record));
            }
        }
    }

    if (reader.ok())
    {
        const auto obligationCount = reader.cappedCount("obligation count");
        snapshot.obligations.reserve(obligationCount);
        for (std::uint32_t i = 0; reader.ok() && i < obligationCount; ++i)
        {
            Obligation obligation;
            obligation.status  = reader.enumValue<Obligation::Status>("obligation status", 5);
            obligation.trigger = reader.enumValue<Obligation::TriggerKind>("obligation trigger", 5);
            if (!reader.ok())
            {
                break;
            }
            obligation.id                 = reader.i64("obligation id");
            obligation.statement          = reader.str("obligation statement");
            obligation.completionCriteria = reader.str("obligation completion");
            snapshot.obligations.push_back(std::move(obligation));
        }
    }

    if (reader.ok())
    {
        const auto evidenceCount = reader.cappedCount("evidence count");
        snapshot.evidence.reserve(evidenceCount);
        for (std::uint32_t i = 0; reader.ok() && i < evidenceCount; ++i)
        {
            EvidenceRecord evidence;
            evidence.boundTo = reader.str("evidence boundTo");
            evidence.content = reader.str("evidence content");
            if (reader.ok())
            {
                snapshot.evidence.push_back(std::move(evidence));
            }
        }
    }

    if (reader.ok())
    {
        const auto conflictCount = reader.cappedCount("conflict count");
        snapshot.conflicts.reserve(conflictCount);
        for (std::uint32_t i = 0; reader.ok() && i < conflictCount; ++i)
        {
            Conflict conflict;
            conflict.id     = reader.str("conflict id");
            conflict.status = reader.enumValue<Conflict::Status>("conflict status", 1);
            if (!reader.ok())
            {
                break;
            }
            conflict.scope       = reader.str("conflict scope");
            conflict.description = reader.str("conflict description");
            conflict.resolution  = reader.str("conflict resolution");
            snapshot.conflicts.push_back(std::move(conflict));
        }
    }

    if (reader.ok())
    {
        const auto profileCount = reader.cappedCount("profile count");
        snapshot.profiles.reserve(profileCount);
        for (std::uint32_t i = 0; reader.ok() && i < profileCount; ++i)
        {
            ProfileRecord profile;
            profile.id      = reader.str("profile id");
            profile.kind    = reader.str("profile kind");
            profile.summary = reader.str("profile summary");
            snapshot.profiles.push_back(std::move(profile));
        }
    }

    if (reader.ok())
    {
        const auto viewCount = reader.cappedCount("view count");
        snapshot.views.reserve(viewCount);
        for (std::uint32_t i = 0; reader.ok() && i < viewCount; ++i)
        {
            ViewSpec view;
            view.id          = reader.str("view id");
            view.agent       = reader.str("view agent");
            view.human       = reader.str("view human");
            view.summary     = reader.str("view summary");
            view.profileRefs = reader.stringVector("view profile refs");
            if (reader.ok())
            {
                snapshot.views.push_back(std::move(view));
            }
        }
    }

    if (reader.ok() && reader.offset != bytes.size())
    {
        reader.error = DeserializeError { DeserializeError::Kind::Truncated, reader.offset,
                                          "trailing bytes after snapshot" };
    }

    if (reader.error)
    {
        result.ok       = false;
        result.error    = *reader.error;
        result.snapshot = Snapshot {};
        return result;
    }
    result.ok = true;
    return result;
}

Constitution makeConstitution()
{
    Constitution constitution; // the 18 article titles; canonical full texts are
                               // referenced content (DR-007 completion is phased)
    constitution.articles = {
        "Project cognition must outlive its participants",
        "Capture conservatively, canonicalize deliberately, retrieve selectively",
        "Evidence and interpretation are different",
        "History is not overwritten",
        "Session independence is mandatory",
        "Unfinished cognition is first-class",
        "Negative knowledge is first-class",
        "Epistemic types must not be collapsed",
        "Provenance is required",
        "Authority is question-scoped",
        "Conflict is transient epistemic state",
        "Governance authority is explicit",
        "Derived indexes are never canonical",
        "Private does not mean secret store",
        "Simplicity must be falsifiable, not assumed sufficient",
        "Retrieval reliability includes invocation as well as ranking",
        "Validation proves a candidate; it is not an apprenticeship loop",
        "Continuity is testable",
    };
    return constitution;
}

PolicyTable makeDefaultPolicy()
{ // the ADR-0036 classification table + the recovery ladder, as cognition data
    PolicyTable policy;
    policy.handoff = {
        { OperationClass::DecisionAcceptance, true, false }, // merge-class: H2 mandatory
        { OperationClass::MemoryWrite, false, false },
        { OperationClass::ObligationWrite, false, false },
        { OperationClass::StateUpdate, false, false },
        { OperationClass::ConflictWrite, false, false },
        { OperationClass::EvidenceWrite, false, false },
    };
    policy.recovery = {
        { RefusalReason::StaleBase, RecoveryAction::RereadRethink },
        { RefusalReason::HandoffMissing, RecoveryAction::HaltEscalate },
        { RefusalReason::HandoffInvalid, RecoveryAction::HaltEscalate },
        { RefusalReason::UnattendedMutation, RecoveryAction::DeferToSupervised },
        { RefusalReason::InvariantFailed, RecoveryAction::DesignReview },
        { RefusalReason::UnverifiedActor, RecoveryAction::HaltEscalate },
        { RefusalReason::GrantRefused, RecoveryAction::FailClosed },
        { RefusalReason::GovernanceDenied, RecoveryAction::FailClosed },
        { RefusalReason::StoreDiverged, RecoveryAction::FailClosed },
        { RefusalReason::OutcomeUnresolved, RecoveryAction::Block },
        { RefusalReason::KeyConflict, RecoveryAction::FailClosed },
        { RefusalReason::ConflictUnresolved, RecoveryAction::FailClosed },
    };
    return policy;
}

Snapshot makeGenesis()
{
    Snapshot genesis;
    genesis.governance.rootPrincipal = "github:JasonHuang3D";
    genesis.constitution             = makeConstitution();
    genesis.policy                   = makeDefaultPolicy();
    genesis.state.objective          = "qiven-context continuity and kernel design";
    genesis.state.candidateRef       = "not-created";
    return genesis;
}

const Decision* findDecision(const Snapshot& snapshot, std::int64_t id)
{
    for (const auto& decision : snapshot.decisions)
    {
        if (decision.id == id)
        {
            return &decision;
        }
    }
    return nullptr;
}

bool reachesDecision(const Snapshot& snapshot, const Decision& from, std::int64_t target)
{ // acyclic supersession: the chain from `from` must not reach `target` (P-36)
    if (from.id == target)
    {
        return true;
    }
    for (const auto id : from.supersedes)
    {
        const Decision* prior = findDecision(snapshot, id);
        if (prior != nullptr && reachesDecision(snapshot, *prior, target))
        {
            return true;
        }
    }
    return false;
}

const Obligation* findObligation(const Snapshot& snapshot, std::int64_t id)
{
    for (const auto& obligation : snapshot.obligations)
    {
        if (obligation.id == id)
        {
            return &obligation;
        }
    }
    return nullptr;
}

const Conflict* findConflict(const Snapshot& snapshot, const std::string& id)
{
    for (const auto& conflict : snapshot.conflicts)
    {
        if (conflict.id == id)
        {
            return &conflict;
        }
    }
    return nullptr;
}

const ProfileRecord* findProfile(const Snapshot& snapshot, const std::string& id)
{
    for (const auto& profile : snapshot.profiles)
    {
        if (profile.id == id)
        {
            return &profile;
        }
    }
    return nullptr;
}

std::uint64_t fnv1a(const std::string& text, const Bytes& bytes)
{
    std::uint64_t hash = 1469598103934665603ULL; // FNV-1a 64 — draft stand-in for SHA-256
    for (const char ch : text)
    {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(ch));
        hash *= 1099511628211ULL;
    }
    for (const std::byte byte : bytes)
    {
        hash ^= static_cast<std::uint64_t>(std::to_integer<unsigned char>(byte));
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string hex64(std::uint64_t hash)
{
    char text[17];
    std::snprintf(text, sizeof text, "%016llx", static_cast<unsigned long long>(hash));
    return std::string(text);
}
} // namespace

ContentId DraftContentId(const Bytes& stateBytes)
{
    return "draft-" + hex64(fnv1a("", stateBytes));
}

SnapshotDigest DraftSnapshotDigest(const Bytes& stateBytes)
{
    return SnapshotDigest { "snap-" + hex64(fnv1a("", stateBytes)) };
}

ContentId DigestOperations(const std::vector<Operation>& operations)
{ // the exact delta an H2 review binds to (DR-004): ordered fields, no framing
    Bytes bytes;
    for (const auto& operation : operations)
    {
        putU8(bytes, static_cast<std::uint8_t>(operation.kind));
        putI64(bytes, operation.recordId);
        putI64(bytes, operation.successorRecordId);
        putU8(bytes, operation.aux);
        putStr(bytes, operation.scope);
        putStr(bytes, operation.title);
        putStr(bytes, operation.payload);
        putStr(bytes, operation.provenanceRef);
    }
    return DraftContentId(bytes);
}

Bytes SerializeSnapshot(const Snapshot& snapshot)
{
    return serializeImpl(snapshot);
}

namespace
{
[[nodiscard]] std::string RequestDigestOf(const ContextTransaction& transaction)
{ // digest of the complete request: base, ordered operations, evidence (ADR-0033 §4)
    Bytes bytes;
    putStr(bytes, transaction.base.value);
    putStr(bytes, DigestOperations(transaction.operations));
    putStr(bytes, transaction.handoffEvidenceRef);
    putU8(bytes, transaction.h2.has_value() ? 1 : 0);
    if (transaction.h2.has_value())
    {
        putStr(bytes, transaction.h2->reviewedDeltaDigest);
        putStr(bytes, transaction.h2->reviewer);
        putStr(bytes, transaction.h2->reviewerBinding);
        putStr(bytes, transaction.h2->reviewRef);
    }
    return DraftContentId(bytes);
}
} // namespace

// --- MemoryStore: the contract, proven ---------------------------------------

StoreReceipt MemoryStore::compareAndSwap(const RevisionId& base, const Bytes& stateBytes)
{
    if (!revisions_.empty() && base != head_)
    {
        return StoreReceipt { StoreReceipt::Kind::CompareFailed, {} }; // definitely not committed
    }
    if (!revisions_.empty() && revisions_.back().second == stateBytes)
    {
        // idempotent re-commit: the head already holds exactly these bytes
        return StoreReceipt { StoreReceipt::Kind::Committed, head_ };
    }
    const RevisionId id { "rev-" + hex64(fnv1a(base.value, stateBytes)) };
    revisions_.emplace_back(id, stateBytes);
    head_ = id;
    return StoreReceipt { StoreReceipt::Kind::Committed, id };
}

Bytes MemoryStore::materialize(const RevisionId& revision) const
{
    for (const auto& [key, stateBytes] : revisions_)
    {
        if (key == revision)
        {
            return stateBytes;
        }
    }
    return {};
}

bool MemoryStore::verify(const RevisionId& revision) const
{
    for (const auto& [key, stateBytes] : revisions_)
    {
        static_cast<void>(stateBytes);
        if (key == revision)
        {
            return true;
        }
    }
    return false;
}

RevisionId MemoryStore::head() const
{
    return head_;
}

// --- GitStore: documented sketch, deliberately unimplemented -----------------

StoreReceipt GitStore::compareAndSwap(const RevisionId& base, const Bytes& stateBytes)
{
    static_cast<void>(base);
    static_cast<void>(stateBytes);
    throw std::logic_error("qiven-context-draft: git transport is a documented sketch");
}

Bytes GitStore::materialize(const RevisionId& revision) const
{
    static_cast<void>(revision);
    throw std::logic_error("qiven-context-draft: git transport is a documented sketch");
}

bool GitStore::verify(const RevisionId& revision) const
{
    static_cast<void>(revision);
    throw std::logic_error("qiven-context-draft: git transport is a documented sketch");
}

RevisionId GitStore::head() const
{
    throw std::logic_error("qiven-context-draft: git transport is a documented sketch");
}

// --- the identity port (draft implementation) ---------------------------------

namespace
{
class RootPrincipalVerifier final : public IIdentityVerifier
{
public:
    // draft port: the actor must name the governance principal carried by the
    // snapshot the service supplies, plus the serving-model disclosure. The
    // port never re-enters QivenContext (P-42); production verifies against
    // the live identity provider and re-checks at commit (ADR-0033 section 4).
    [[nodiscard]] bool verify(const AuthenticatedActor& actor,
                              const Snapshot& governanceSource) const override
    {
        return !actor.principal.empty() && !actor.binding.empty() && !actor.servingModel.empty() && actor.principal == governanceSource.governance.rootPrincipal;
    }
};
} // namespace

// --- QivenContext: the gated service -----------------------------------------

std::mutex QivenContext::g_admission;
std::atomic<Epoch> QivenContext::g_epoch { 0 };
std::shared_ptr<ICognitionStore> QivenContext::g_store;
std::shared_ptr<IIdentityVerifier> QivenContext::g_identity;
std::vector<QivenContext::Registry> QivenContext::g_live;
GrantId QivenContext::g_activeGrantId;
AuthenticatedActor QivenContext::g_grantActor;
std::map<std::string, QivenContext::Receipt> QivenContext::g_receipts;

ExecutionGrant::ExecutionGrant(GrantId id, Epoch epoch, PrincipalId principal, WorkMode mode) :
id_(std::move(id)), epoch_(epoch), principal_(std::move(principal)), mode_(mode)
{
}

void QivenContext::attachStore(std::shared_ptr<ICognitionStore> store)
{
    std::lock_guard lock(g_admission);
    g_store = std::move(store);
}

void QivenContext::attachIdentityVerifier(std::shared_ptr<IIdentityVerifier> verifier)
{
    std::lock_guard lock(g_admission);
    g_identity = verifier ? std::move(verifier) : std::make_shared<RootPrincipalVerifier>();
}

CognitionHandle QivenContext::CreateCognition(const CognitionSource& source, DeserializeError* err)
{
    std::lock_guard lock(g_admission);
    QIVEN_ASSERT(g_store != nullptr); // draft precondition: attachStore before boot

    auto fail = [&](DeserializeError error) -> CognitionHandle {
        if (err)
        {
            *err = error;
        }
        return nullptr;
    };

    Bytes bytes;
    RevisionId revision; // a quarantined artifact has NO storage identity
    QuarantineState quarantine { QuarantineState::Promoted };
    switch (source.kind)
    {
    case CognitionSourceKind::CanonicalRemote:
    {
        const RevisionId id = IsEmpty(source.revision) ? g_store->head() : source.revision;
        bytes               = g_store->materialize(id);
        if (bytes.empty())
        {
            // genesis: no canonical state yet — materialize the empty cognition
            // (governance + constitution + default policy) and append it as state #1
            bytes               = serializeImpl(makeGenesis());
            const auto appended = g_store->compareAndSwap(RevisionId {}, bytes);
            QIVEN_ASSERT(appended.kind == StoreReceipt::Kind::Committed);
            revision = appended.revision;
        }
        else
        {
            revision = id;
        }
        break;
    }
    case CognitionSourceKind::HandoffArtifact:
        bytes      = source.inlineBytes;        // K4: restored from artifact bytes alone
        quarantine = QuarantineState::Isolated; // restore is not authority
        if (!source.expectedDigest.empty() && DraftContentId(bytes) != source.expectedDigest)
        {
            // content-identified delivery: a mismatch is corruption, not a
            // warning (context-handoff-contract: corruption is a failure)
            return fail(DeserializeError { DeserializeError::Kind::DigestMismatch, 0,
                                           "artifact digest mismatch" });
        }
        break;
    case CognitionSourceKind::CompressedStream:
        throw std::logic_error("qiven-context-draft: K5 compressed transport not implemented");
    }
    if (bytes.empty())
    {
        return fail(DeserializeError { DeserializeError::Kind::Truncated, 0, "empty source" });
    }

    const auto parsed = deserializeImpl(bytes);
    if (!parsed.ok)
    {
        return fail(parsed.error); // corruption fails closed; never an assert (DR-009)
    }

    const Epoch epoch  = g_epoch.fetch_add(1) + 1; // fencing token per materialization
    auto minted        = std::make_shared<Materialization>();
    minted->state      = std::make_shared<const Snapshot>(std::move(parsed.snapshot));
    minted->digest     = DraftSnapshotDigest(bytes); // integrity identity
    minted->revision   = revision;                   // storage identity (empty for artifacts)
    minted->epoch      = epoch;
    minted->quarantine = quarantine;

    g_live.push_back(Registry { minted });
    return CognitionHandle { minted }; // participants get the immutable handle
}

Data QivenContext::ReadFromCognition(const CognitionHandle& handle, const Query& query)
{
    static_cast<void>(query); // typed Bundle with floors lands in Phase 2 (DR-007)
    QIVEN_ASSERT(handle != nullptr);
    // no admission lock needed: the materialization is IMMUTABLE (review §2) —
    // reading a pinned handle can never race the publish path
    return serializeImpl(*handle->state);
}

std::optional<ExecutionGrant> QivenContext::AcquireGrant(const AuthenticatedActor& actor, WorkMode mode)
{
    std::lock_guard lock(g_admission);
    if (!IsEmpty(g_activeGrantId))
    {
        return std::nullopt; // split-brain: a second flow is refused, not queued (P-01)
    }
    if (!g_identity)
    {
        g_identity = std::make_shared<RootPrincipalVerifier>();
    }
    const auto governance = CanonicalHeadLocked(); // context passed IN — no re-entry (P-42)
    if (!governance || !g_identity->verify(actor, *governance))
    {
        return std::nullopt; // identity is never caller-asserted (P-02)
    }
    static std::uint64_t grantSequence = 0;
    const Bytes sequence { std::byte { static_cast<unsigned char>((++grantSequence) & 0xFF) } };
    const GrantId id { "grant-" + hex64(fnv1a(actor.principal + "|" + actor.binding, sequence)) };
    g_activeGrantId = id;
    g_grantActor    = actor;
    return ExecutionGrant { id, g_epoch.load(), actor.principal, mode };
}

void QivenContext::ReleaseGrant(const ExecutionGrant& grant)
{
    std::lock_guard lock(g_admission);
    if (g_activeGrantId == grant.id())
    {
        g_activeGrantId = GrantId {};
        g_grantActor    = AuthenticatedActor {};
    }
}

Verdict QivenContext::WriteToCognition(const CognitionHandle& handle,
                                       const ContextTransaction& transaction,
                                       const ExecutionGrant& grant)
{
    std::lock_guard lock(g_admission);
    const auto refuse = [](RefusalReason reason) {
        return Verdict { Verdict::Outcome::Refused, reason, {} };
    };

    // gate 3: idempotency key resolution (DR-011). A same-key same-request
    // replay returns the original durable verdict without re-executing — even
    // when the base has since gone stale; same key with a different request is
    // refused. Keys without receipts behave as before.
    const std::string requestDigest = transaction.idempotencyKey.empty()
                                          ? std::string {}
                                          : RequestDigestOf(transaction);
    if (!transaction.idempotencyKey.empty())
    {
        const auto receipt = g_receipts.find(transaction.idempotencyKey);
        if (receipt != g_receipts.end())
        {
            if (receipt->second.requestDigest != requestDigest)
            {
                return refuse(RefusalReason::KeyConflict); // same key, different request
            }
            return receipt->second.outcome; // durable outcome, no re-execution
        }
    }

    Registry* entry = nullptr; // gate 0: known handle, non-quarantined
    for (auto& live : g_live)
    {
        if (live.materialization == handle)
        {
            entry = &live;
            break;
        }
    }
    if (entry == nullptr || entry->materialization->quarantine != QuarantineState::Promoted)
    {
        return refuse(RefusalReason::GovernanceDenied); // pit P-33: restore never self-promotes
    }
    if (IsEmpty(g_activeGrantId) || g_activeGrantId != grant.id())
    {
        return refuse(RefusalReason::GrantRefused); // stale or foreign grants never regain authority
    }
    const auto governance = CanonicalHeadLocked(); // context passed IN — no re-entry (P-42)
    if (!governance || !g_identity || !g_identity->verify(g_grantActor, *governance))
    {
        return refuse(RefusalReason::UnverifiedActor); // re-checked at commit (ADR-0033 §4)
    }

    if (grant.mode() == WorkMode::Unattended && !transaction.operations.empty())
    {
        return refuse(RefusalReason::UnattendedMutation); // ADR-0036: read-only default
    }
    if (transaction.base != entry->materialization->revision)
    {
        return refuse(RefusalReason::StaleBase); // durable divergence: re-read (P-25 family)
    }

    if (transaction.operations.empty())
    {
        return Verdict { Verdict::Outcome::Applied, RefusalReason::StoreDiverged,
                         handle }; // ordinary turn: nothing to store
    }

    // gates 6-7: policy table + AUTHORITY-BOUND H2 + conflict scope, for the
    // WHOLE transaction before anything applies (no partial snapshot)
    for (const auto& operation : transaction.operations)
    {
        OperationClass opClass = OperationClass::StateUpdate;
        switch (operation.kind)
        {
        case Operation::Kind::AppendDecision:
        case Operation::Kind::SupersedeDecision: opClass = OperationClass::DecisionAcceptance; break;
        case Operation::Kind::AddMemory: opClass = OperationClass::MemoryWrite; break;
        case Operation::Kind::UpsertObligation:
        case Operation::Kind::CloseObligation:
        case Operation::Kind::TransitionObligation: opClass = OperationClass::ObligationWrite; break;
        case Operation::Kind::UpdateState:
        case Operation::Kind::SetNextBoundary: opClass = OperationClass::StateUpdate; break;
        case Operation::Kind::OpenConflict:
        case Operation::Kind::ResolveConflict: opClass = OperationClass::ConflictWrite; break;
        case Operation::Kind::AddEvidence:
        case Operation::Kind::UpsertProfile:
        case Operation::Kind::AmendViewSpec: opClass = OperationClass::EvidenceWrite; break;
        }
        const HandoffPolicy* row = nullptr;
        for (const auto& candidate : entry->materialization->state->policy.handoff)
        {
            if (candidate.opClass == opClass)
            {
                row = &candidate;
                break;
            }
        }
        if (row == nullptr)
        {
            return refuse(RefusalReason::GovernanceDenied); // unregistered operation class
        }
        if (row->requiresH2)
        {
            if (!transaction.h2.has_value())
            {
                return refuse(RefusalReason::HandoffMissing); // no-verbal-waiver: no retry
            }
            // review §5: H2 is authority-bound, not only content-bound
            if (transaction.h2->reviewer.empty() || transaction.h2->reviewerBinding.empty() || transaction.h2->reviewRef.empty() || transaction.handoffEvidenceRef.empty())
            {
                return refuse(RefusalReason::HandoffMissing);
            }
            if (transaction.h2->reviewerBinding == g_grantActor.binding)
            {
                return refuse(RefusalReason::HandoffInvalid); // self-review ban (P-09)
            }
            if (transaction.h2->reviewer != governance->governance.rootPrincipal)
            {
                return refuse(RefusalReason::HandoffInvalid); // reviewer holds no H2 authority
            }
            if (transaction.h2->reviewedDeltaDigest != DigestOperations(transaction.operations))
            {
                return refuse(RefusalReason::HandoffInvalid); // evidence not bound to THIS delta
            }
        }
    }

    const Snapshot& current = *entry->materialization->state;

    // gate 7.5: an OPEN conflict whose scope intersects the operation blocks
    // it (DR-006) — the K4-trial-1 lesson as a compiled gate
    for (const auto& operation : transaction.operations)
    {
        if (operation.kind == Operation::Kind::OpenConflict || operation.kind == Operation::Kind::ResolveConflict)
        {
            continue;
        }
        for (const auto& conflict : current.conflicts)
        {
            if (conflict.status == Conflict::Status::Open && (conflict.scope.empty() || conflict.scope == operation.scope))
            {
                return refuse(RefusalReason::ConflictUnresolved);
            }
        }
    }

    // gate 8: invariants for the WHOLE transaction
    for (const auto& operation : transaction.operations)
    {
        switch (operation.kind)
        {
        case Operation::Kind::AppendDecision:
            if (operation.recordId <= 0 || operation.title.empty() || operation.payload.empty())
            {
                return refuse(RefusalReason::InvariantFailed);
            }
            for (const auto& existing : current.decisions)
            {
                if (existing.id == operation.recordId)
                {
                    return refuse(RefusalReason::InvariantFailed); // ids are never reused
                }
            }
            break;
        case Operation::Kind::SupersedeDecision:
        {
            const Decision* superseded = findDecision(current, operation.recordId);
            const Decision* successor  = findDecision(current, operation.successorRecordId);
            if (superseded == nullptr || successor == nullptr || superseded == successor || superseded->status != Lifecycle::Accepted || successor->status != Lifecycle::Accepted)
            {
                return refuse(RefusalReason::InvariantFailed);
            }
            if (reachesDecision(current, *successor, operation.recordId))
            {
                return refuse(RefusalReason::InvariantFailed); // acyclic supersession (P-36)
            }
            if (operation.payload.empty())
            {
                return refuse(RefusalReason::InvariantFailed); // transition rationale required
            }
            break;
        }
        case Operation::Kind::AddMemory:
            if (operation.payload.empty() || operation.provenanceRef.empty() || operation.aux > 5)
            {
                return refuse(RefusalReason::InvariantFailed); // pit P-37: no unprovenanced lessons
            }
            break;
        case Operation::Kind::UpsertObligation:
            if (operation.recordId <= 0 || operation.payload.empty())
            {
                return refuse(RefusalReason::InvariantFailed);
            }
            break;
        case Operation::Kind::CloseObligation:
        {
            bool foundOpen = false;
            for (const auto& obligation : current.obligations)
            {
                if (obligation.id == operation.recordId && obligation.status == Obligation::Status::Open)
                {
                    foundOpen = true;
                    break;
                }
            }
            if (!foundOpen)
            {
                return refuse(RefusalReason::InvariantFailed);
            }
            break;
        }
        case Operation::Kind::TransitionObligation:
        {
            const Obligation* obligation = findObligation(current, operation.recordId);
            if (obligation == nullptr || obligation->status != Obligation::Status::Open || operation.payload.empty() || operation.aux > 5)
            {
                return refuse(RefusalReason::InvariantFailed);
            }
            if (operation.aux == 0 || operation.aux == 2)
            {
                return refuse(RefusalReason::InvariantFailed); // Open/Blocked are not transitions
            }
            if (operation.aux == 5 && findObligation(current, operation.successorRecordId) == nullptr)
            {
                return refuse(RefusalReason::InvariantFailed); // superseded needs a successor
            }
            break;
        }
        case Operation::Kind::UpdateState:
            if (operation.payload.empty())
            {
                return refuse(RefusalReason::InvariantFailed);
            }
            break;
        case Operation::Kind::AddEvidence:
            if (operation.title.empty() || operation.payload.empty())
            {
                return refuse(RefusalReason::InvariantFailed);
            }
            break;
        case Operation::Kind::UpsertProfile:
            if (operation.title.empty() || operation.scope.empty() || operation.payload.empty())
            {
                return refuse(RefusalReason::InvariantFailed); // id + kind + summary required
            }
            if (findProfile(current, operation.title) != nullptr)
            {
                return refuse(RefusalReason::InvariantFailed); // profile ids never reused
            }
            break;
        case Operation::Kind::OpenConflict:
            if (operation.title.empty() || operation.payload.empty())
            {
                return refuse(RefusalReason::InvariantFailed); // id + description required
            }
            for (const auto& conflict : current.conflicts)
            {
                if (conflict.id == operation.title)
                {
                    return refuse(RefusalReason::InvariantFailed); // ids are never reused
                }
            }
            break;
        case Operation::Kind::ResolveConflict:
        {
            const Conflict* conflict = findConflict(current, operation.title);
            if (conflict == nullptr || conflict->status != Conflict::Status::Open || operation.payload.empty())
            {
                return refuse(RefusalReason::InvariantFailed); // resolution text required
            }
            break;
        }
        case Operation::Kind::AmendViewSpec:
        {
            // pit.view_refs_resolve: empty ids, empty refs or duplicated profile
            // refs fail view compilation — refused at the gate, not at read time
            if (operation.title.empty() || operation.scope.empty() || operation.scope.find(':') == std::string::npos)
            {
                return refuse(RefusalReason::InvariantFailed); // id and agent:human required
            }
            for (const auto& existing : current.views)
            {
                if (existing.id == operation.title && operation.aux == 0)
                {
                    return refuse(RefusalReason::InvariantFailed); // view ids never reused
                }
            }
            std::vector<std::string> refs;
            {
                std::istringstream parsed { operation.provenanceRef };
                std::string ref;
                while (std::getline(parsed, ref, ','))
                {
                    if (!ref.empty())
                    {
                        refs.push_back(ref);
                    }
                }
            }
            if (refs.empty())
            {
                return refuse(RefusalReason::InvariantFailed); // a view without profiles is invention
            }
            // review §7: refs must RESOLVE in-snapshot, not merely be non-empty
            for (const auto& ref : refs)
            {
                if (findProfile(current, ref) == nullptr)
                {
                    return refuse(RefusalReason::InvariantFailed); // dangling profile ref
                }
            }
            for (std::size_t i = 0; i < refs.size(); ++i)
            {
                for (std::size_t k = i + 1; k < refs.size(); ++k)
                {
                    if (refs[i] == refs[k])
                    {
                        return refuse(RefusalReason::InvariantFailed); // duplicated ref
                    }
                }
            }
            break;
        }
        case Operation::Kind::SetNextBoundary:
        {
            const Obligation* boundary = findObligation(current, operation.recordId);
            if (boundary == nullptr || boundary->status != Obligation::Status::Open)
            {
                return refuse(RefusalReason::InvariantFailed); // the boundary binds to OPEN work
            }
            break;
        }
        }
    }

    // apply atomically onto a successor snapshot (DR-001: the current tree is
    // never mutated in place; the successor is published whole or not at all)
    Snapshot next = current;
    for (const auto& operation : transaction.operations)
    {
        switch (operation.kind)
        {
        case Operation::Kind::AppendDecision:
            next.decisions.push_back(Decision { operation.recordId,
                                                Lifecycle::Accepted,
                                                operation.title,
                                                operation.payload,
                                                {},
                                                {},
                                                Provenance { { transaction.handoffEvidenceRef } } });
            break;
        case Operation::Kind::SupersedeDecision:
            for (auto& decision : next.decisions)
            {
                if (decision.id == operation.recordId)
                {
                    decision.status = Lifecycle::Superseded; // history kept, status moves
                    decision.supersededBy.push_back(operation.successorRecordId);
                }
                if (decision.id == operation.successorRecordId)
                {
                    decision.supersedes.push_back(operation.recordId); // reciprocal, acyclic
                }
            }
            break;
        case Operation::Kind::AddMemory:
            // aux carries the MemoryRecord::Kind: negative knowledge is a floor
            next.memory.push_back(MemoryRecord { static_cast<MemoryRecord::Kind>(operation.aux),
                                                 MemoryRecord::Status::Active,
                                                 operation.title,
                                                 operation.payload,
                                                 Provenance { { operation.provenanceRef } } });
            break;
        case Operation::Kind::UpsertObligation:
            next.obligations.push_back(Obligation { Obligation::Status::Open,
                                                    Obligation::TriggerKind::Manual,
                                                    operation.recordId,
                                                    operation.payload,
                                                    {} });
            break;
        case Operation::Kind::CloseObligation:
            for (auto& obligation : next.obligations)
            {
                if (obligation.id == operation.recordId)
                {
                    obligation.status = Obligation::Status::Done; // history kept, status moves
                    break;
                }
            }
            break;
        case Operation::Kind::TransitionObligation:
            for (auto& obligation : next.obligations)
            {
                if (obligation.id == operation.recordId)
                {
                    obligation.status             = static_cast<Obligation::Status>(operation.aux);
                    obligation.completionCriteria = operation.payload;
                }
            }
            break;
        case Operation::Kind::UpdateState:
            next.state.current = operation.payload;
            break;
        case Operation::Kind::AddEvidence:
            next.evidence.push_back(EvidenceRecord { operation.title, operation.payload });
            break;
        case Operation::Kind::UpsertProfile:
            next.profiles.push_back(
                ProfileRecord { operation.title, operation.scope, operation.payload });
            break;
        case Operation::Kind::OpenConflict:
            next.conflicts.push_back(Conflict { operation.title,
                                                Conflict::Status::Open,
                                                operation.scope,
                                                operation.payload,
                                                {} });
            break;
        case Operation::Kind::ResolveConflict:
            for (auto& conflict : next.conflicts)
            {
                if (conflict.id == operation.title)
                {
                    conflict.status     = Conflict::Status::Resolved;
                    conflict.resolution = operation.payload;
                }
            }
            break;
        case Operation::Kind::AmendViewSpec:
        {
            if (operation.aux == 1)
            { // retirement: the view is dropped by id (history stays in past snapshots)
                next.views.erase(std::remove_if(next.views.begin(), next.views.end(),
                                                [&](const ViewSpec& view) { return view.id == operation.title; }),
                                 next.views.end());
                break;
            }
            ViewSpec view;
            view.id      = operation.title;
            view.agent   = operation.scope.substr(0, operation.scope.find(':'));
            view.human   = operation.scope.substr(operation.scope.find(':') + 1);
            view.summary = operation.payload;
            std::istringstream refs { operation.provenanceRef };
            std::string ref;
            while (std::getline(refs, ref, ','))
            {
                if (!ref.empty())
                {
                    view.profileRefs.push_back(ref);
                }
            }
            next.views.push_back(std::move(view));
            break;
        }
        case Operation::Kind::SetNextBoundary:
            next.state.nextBoundaryRef = std::to_string(operation.recordId);
            break;
        }
    }

    // gate 9: post-apply state coherence (pit P-15) — a nextBoundary that no
    // longer resolves to OPEN work refuses the whole transaction
    if (!next.state.nextBoundaryRef.empty())
    {
        bool boundaryResolved = false;
        for (const auto& obligation : next.obligations)
        {
            if (std::to_string(obligation.id) == next.state.nextBoundaryRef && obligation.status == Obligation::Status::Open)
            {
                boundaryResolved = true;
                break;
            }
        }
        if (!boundaryResolved)
        {
            return refuse(RefusalReason::InvariantFailed); // dangling boundary reference
        }
    }

    // gate 10: store compareAndSwap on the REVISION — a CompareFailed is a
    // definite rejection; an OutcomeUnknown is a lost acknowledgement and can
    // never be claimed as a rollback (review §6)
    const Bytes newState     = serializeImpl(next);
    const auto storeReceipt  = g_store->compareAndSwap(entry->materialization->revision, newState);
    const auto finishUnknown = [&]() {
        const Verdict unknown { Verdict::Outcome::OutcomeUnknown,
                                RefusalReason::OutcomeUnresolved,
                                {} };
        if (!transaction.idempotencyKey.empty())
        {
            g_receipts[transaction.idempotencyKey] = Receipt { requestDigest, unknown };
        }
        return unknown;
    };
    if (storeReceipt.kind == StoreReceipt::Kind::CompareFailed)
    {
        return refuse(RefusalReason::StoreDiverged); // store-level CAS failed: fail closed
    }
    if (storeReceipt.kind == StoreReceipt::Kind::OutcomeUnknown)
    {
        return finishUnknown();
    }

    // publish the SUCCESSOR MATERIALIZATION (review §2): minted immutable, the
    // registry advances to it; the caller's pinned handle keeps its world
    auto successor         = std::make_shared<Materialization>();
    successor->state       = std::make_shared<const Snapshot>(std::move(next));
    successor->digest      = DraftSnapshotDigest(newState);
    successor->revision    = storeReceipt.revision;
    successor->epoch       = entry->materialization->epoch; // same lineage, same lease
    successor->quarantine  = entry->materialization->quarantine;
    entry->materialization = successor;

    const Verdict applied { Verdict::Outcome::Applied, RefusalReason::StoreDiverged,
                            CognitionHandle { successor } };
    if (!transaction.idempotencyKey.empty())
    {
        g_receipts[transaction.idempotencyKey] = Receipt { requestDigest, applied };
    }
    return applied;
}

RecoveryAction QivenContext::RecoveryFor(const Snapshot& snapshot, RefusalReason reason)
{
    for (const auto& rule : snapshot.policy.recovery)
    {
        if (rule.reason == reason)
        {
            return rule.action; // recovery-as-data (DR-002): the loop obeys the table
        }
    }
    return RecoveryAction::FailClosed; // unknown refusal: never invent a recovery
}

Epoch QivenContext::currentEpoch()
{
    return g_epoch.load(); // lock-free sampling: cheap staleness pre-checks
}

bool QivenContext::RetireCognition(const CognitionHandle& handle)
{
    std::lock_guard lock(g_admission);
    for (std::size_t i = 0; i < g_live.size(); ++i)
    {
        if (g_live[i].materialization == handle)
        {
            g_live.erase(g_live.begin() + static_cast<std::ptrdiff_t>(i));
            return true; // destruction deferred until the last pin releases
        }
    }
    return false;
}

std::shared_ptr<const Snapshot> QivenContext::CanonicalHead()
{
    std::lock_guard lock(g_admission);
    return CanonicalHeadLocked();
}

std::shared_ptr<const Snapshot> QivenContext::CanonicalHeadLocked()
{
    if (!g_store)
    {
        return nullptr;
    }
    const auto head = g_store->head();
    if (IsEmpty(head))
    {
        return nullptr;
    }
    for (const auto& entry : g_live)
    {
        if (entry.materialization->revision == head && entry.materialization->quarantine == QuarantineState::Promoted)
        {
            return entry.materialization->state; // fast path: the live canonical materialization
        }
    }
    const auto parsed = deserializeImpl(g_store->materialize(head));
    if (!parsed.ok)
    {
        return nullptr;
    }
    return std::make_shared<const Snapshot>(std::move(parsed.snapshot));
}

std::optional<ViewSpec> QivenContext::ResolveView(const CognitionHandle& handle,
                                                  const std::string& viewId,
                                                  ResolveDiagnostic* diag)
{
    QIVEN_ASSERT(handle != nullptr);
    const auto reset = [&](ResolveDiagnostic::Kind kind) -> std::optional<ViewSpec> {
        if (diag)
        {
            diag->kind = kind;
        }
        return std::nullopt;
    };
    for (const auto& view : handle->state->views)
    {
        if (view.id != viewId)
        {
            continue;
        }
        if (view.profileRefs.empty())
        {
            return reset(ResolveDiagnostic::Kind::InvalidRefs); // fails compilation (P-40)
        }
        for (const auto& ref : view.profileRefs)
        { // review §7: refs must RESOLVE in-snapshot
            if (findProfile(*handle->state, ref) == nullptr)
            {
                return reset(ResolveDiagnostic::Kind::InvalidRefs);
            }
        }
        if (diag)
        {
            diag->kind = ResolveDiagnostic::Kind::None;
        }
        return view; // resolved: the participant combination is REAL, never invented
    }
    return reset(ResolveDiagnostic::Kind::NotFound); // S1-R2: fall back, never invent
}

ContextBundle QivenContext::BuildBundle(const CognitionHandle& handle, const Query& query)
{
    QIVEN_ASSERT(handle != nullptr);
    const Snapshot& snapshot = *handle->state;
    ContextBundle bundle;
    bundle.snapshot = handle->revision;

    // SEMANTIC floors: real content, present regardless of any budget
    // (S1-R3, review §8: floor COUNT is not floor CONTENT)
    bundle.mandatoryInputs.push_back("governance: " + snapshot.governance.rootPrincipal);
    for (const auto& article : snapshot.constitution.articles)
    {
        bundle.mandatoryInputs.push_back("constitution: " + article);
    }
    bundle.mandatoryInputs.push_back("objective: " + snapshot.state.objective);
    if (!snapshot.state.nextBoundaryRef.empty())
    {
        const Obligation* boundary =
            findObligation(snapshot, std::stoll(snapshot.state.nextBoundaryRef));
        if (boundary != nullptr)
        {
            bundle.mandatoryInputs.push_back("next boundary: obligation " + snapshot.state.nextBoundaryRef + ": " + boundary->statement);
        }
    }
    for (const auto& conflict : snapshot.conflicts)
    {
        if (conflict.status == Conflict::Status::Open)
        {
            bundle.mandatoryInputs.push_back("OPEN CONFLICT " + conflict.id + ": " + conflict.description);
        }
    }
    for (const auto& record : snapshot.memory)
    { // negative knowledge is a floor: rejected alternatives surface where a
      // pit is about to be re-dug (constitution #7)
        if (record.kind == MemoryRecord::Kind::NegativeKnowledge && record.status == MemoryRecord::Status::Active)
        {
            bundle.mandatoryInputs.push_back("negative knowledge: " + record.statement);
        }
    }

    // protected constraints, derived verbatim from the policy table in cognition
    for (const auto& row : snapshot.policy.handoff)
    {
        if (row.requiresH2)
        {
            bundle.protectedConstraints.push_back(
                "H2 review evidence is mandatory for this operation class");
        }
        if (row.rootPrincipalOnly)
        {
            bundle.protectedConstraints.push_back("root-principal governance class");
        }
    }
    bundle.protectedConstraints.push_back("typed handoffs are never verbally waived (ADR-0036)");
    bundle.protectedConstraints.push_back("unattended automation is read-only (ADR-0036)");

    // candidates: typed, deterministic order, shrunk by an approximate-TOKEN
    // budget only (review §8: a candidate is not "1 token"); dropped material
    // is explained (DR-007)
    const std::size_t budget = query.tokenBudget == 0 ? 512 : query.tokenBudget;
    std::size_t used         = 0;
    const auto estimate      = [](const std::string& text) { return (text.size() + 3) / 4; };
    for (const auto& decision : snapshot.decisions)
    {
        const BundleCandidate candidate { "decision", std::to_string(decision.id),
                                          decision.status == Lifecycle::Accepted ? "accepted: " + decision.title
                                                                                 : "lifecycle: " + decision.title };
        const std::size_t cost = estimate(candidate.summary);
        if (used + cost <= budget)
        {
            bundle.candidates.push_back(candidate);
            used += cost;
        }
        else
        {
            bundle.omissions.push_back({ "decision " + std::to_string(decision.id),
                                         "token budget; floors and constraints were never touched" });
        }
    }
    for (const auto& record : snapshot.memory)
    {
        if (record.status != MemoryRecord::Status::Active || record.kind == MemoryRecord::Kind::NegativeKnowledge)
        {
            continue; // negative knowledge is a floor, not a candidate
        }
        const BundleCandidate candidate { "memory", record.title, record.statement };
        const std::size_t cost = estimate(candidate.summary);
        if (used + cost <= budget)
        {
            bundle.candidates.push_back(candidate);
            used += cost;
        }
        else
        {
            bundle.omissions.push_back({ "memory " + record.title,
                                         "token budget; floors and constraints were never touched" });
        }
    }

    // obligations: three-valued, never silently guessed (ADR-0033 §6)
    for (const auto& obligation : snapshot.obligations)
    {
        switch (obligation.status)
        {
        case Obligation::Status::Done:
        case Obligation::Status::Cancelled:
        case Obligation::Status::Superseded:
            bundle.obligations.push_back({ obligation.id,
                                           ObligationEvaluation::Evaluation::Met });
            break;
        case Obligation::Status::Open:
        case Obligation::Status::Deferred:
            bundle.obligations.push_back({ obligation.id,
                                           ObligationEvaluation::Evaluation::Unknown });
            break;
        case Obligation::Status::Blocked:
            bundle.obligations.push_back({ obligation.id,
                                           ObligationEvaluation::Evaluation::Unmet });
            break;
        }
    }
    return bundle;
}

std::string QivenContext::RenderBundle(const ContextBundle& bundle, OutputView view)
{
    std::string text;
    const std::string eol(1, '\n'); // newline as a char: heredocs eat escapes
    if (view == OutputView::Machine)
    { // deterministic flat rendering; parse this, never the human view
        text += "snapshot=" + bundle.snapshot.value + eol;
        for (const auto& floor : bundle.mandatoryInputs)
        {
            text += "floor|" + floor + eol;
        }
        for (const auto& constraint : bundle.protectedConstraints)
        {
            text += "constraint|" + constraint + eol;
        }
        for (const auto& candidate : bundle.candidates)
        {
            text += "candidate|" + candidate.kind + "|" + candidate.id + "|" + candidate.summary + eol;
        }
        for (const auto& obligation : bundle.obligations)
        {
            text += "obligation|" + std::to_string(obligation.id) + "|" + std::to_string(static_cast<int>(obligation.evaluation)) + eol;
        }
        for (const auto& omission : bundle.omissions)
        {
            text += "omission|" + omission.what + "|" + omission.reason + eol;
        }
        text += "authorization: not_granted" + eol; // without exception (ADR-0033 §6)
        return text;
    }
    text += "[ RUN] bundle " + bundle.snapshot.value + eol;
    text += "[ OK ] floors: " + std::to_string(bundle.mandatoryInputs.size()) + ", constraints: " + std::to_string(bundle.protectedConstraints.size()) + ", candidates: " + std::to_string(bundle.candidates.size()) + ", omissions: " + std::to_string(bundle.omissions.size()) + eol;
    text += "authorization: not_granted" + eol;
    return text;
}

bool QivenContext::IsContinueable(const Human* human, const LLMClientTool* client,
                                  const Device* device, const LLM* llm,
                                  const CognitionHandle& handle)
{
    if (human == nullptr || client == nullptr || device == nullptr || llm == nullptr || !handle)
    {
        return false;
    }
    // review §9: a whole-graph predicate validates the EDGES, not just that
    // each node is individually healthy
    if (client->pCurrentLLM.get() != llm || client->pTargetDevice.get() != device || llm->pCognition != handle)
    {
        return false; // the graph is not the graph it claims to be
    }
    if (client->binding.modelId != llm->name)
    {
        return false; // disclosure drift: the binding names another model (P-48)
    }
    if (g_store == nullptr || IsEmpty(handle->revision) || !g_store->verify(handle->revision))
    {
        return false; // state must be restorable from the store
    }
    if (human->verifiedPrincipal.empty() || human->verifiedPrincipal != handle->state->governance.rootPrincipal)
    {
        return false; // authority present (R4)
    }
    if (!client->operational || !device->reachable)
    {
        return false; // runtime plane alive
    }
    for (const auto& obligation : handle->state->obligations)
    {
        if (obligation.status == Obligation::Status::Blocked)
        {
            return false; // an explicit blocker halts the loop
        }
    }
    return true; // budget remains: open design question (draft)
}
} // namespace qiven::context
