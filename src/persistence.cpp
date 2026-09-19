// ============================================================================
// persistence.cpp — serialization (v2, fail-closed), memory store, and the
// gated service with policy-as-data verdicts
// ============================================================================

#include <qiven/context/persistence.hpp>

#include <qiven/context/runtime.hpp> // IsContinueable inspects participant fields

#include <qiven/contracts.hpp>

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace qiven::context
{
namespace
{
constexpr std::uint8_t kSerializationVersion = 2;
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

    putStr(bytes, snapshot.state.activeWork);
    putStr(bytes, snapshot.state.current);
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
            row.opClass           = reader.enumValue<OperationClass>("handoff op class", 3);
            row.requiresH2        = reader.u8("handoff requiresH2") != 0;
            row.rootPrincipalOnly = reader.u8("handoff rootPrincipalOnly") != 0;
            snapshot.policy.handoff.push_back(row);
        }
        const auto recoveryCount = reader.cappedCount("recovery rows");
        snapshot.policy.recovery.reserve(recoveryCount);
        for (std::uint32_t i = 0; reader.ok() && i < recoveryCount; ++i)
        {
            RecoveryRule row;
            row.reason = reader.enumValue<RefusalReason>("recovery reason", 9);
            row.action = reader.enumValue<RecoveryAction>("recovery action", 5);
            snapshot.policy.recovery.push_back(row);
        }

        if (reader.ok())
        {
            snapshot.state.activeWork   = reader.str("state.activeWork");
            snapshot.state.current      = reader.str("state.current");
            snapshot.state.repositories = reader.str("state.repositories");
            snapshot.state.roadmap      = reader.str("state.roadmap");
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
                               // referenced content (DR-007 completion is Phase 2)
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
    };
    return policy;
}

Snapshot makeGenesis()
{
    Snapshot genesis;
    genesis.governance.rootPrincipal = "github:JasonHuang3D";
    genesis.constitution             = makeConstitution();
    genesis.policy                   = makeDefaultPolicy();
    return genesis;
}
} // namespace

ContentId DraftContentId(const Bytes& stateBytes)
{
    std::uint64_t hash = 1469598103934665603ULL; // FNV-1a 64 — draft stand-in for SHA-256
    for (const std::byte byte : stateBytes)
    {
        hash ^= static_cast<std::uint64_t>(std::to_integer<unsigned char>(byte));
        hash *= 1099511628211ULL;
    }
    char text[17];
    std::snprintf(text, sizeof text, "%016llx", static_cast<unsigned long long>(hash));
    return "draft-" + std::string(text);
}

ContentId DigestOperations(const std::vector<Operation>& operations)
{ // the exact delta an H2 review binds to (DR-004): ordered fields, no framing
    Bytes bytes;
    for (const auto& operation : operations)
    {
        putU8(bytes, static_cast<std::uint8_t>(operation.kind));
        putI64(bytes, operation.recordId);
        putStr(bytes, operation.title);
        putStr(bytes, operation.payload);
    }
    return DraftContentId(bytes);
}

Bytes SerializeSnapshot(const Snapshot& snapshot)
{
    return serializeImpl(snapshot);
}

// --- MemoryStore: the contract, proven ---------------------------------------

ContentId MemoryStore::append(const Bytes& stateBytes, const ContentId& base)
{
    if (!states_.empty() && base != head_)
    {
        return {}; // divergence: appends must chain onto current head
    }
    const ContentId id = DraftContentId(stateBytes);
    if (!states_.empty() && id == head_)
    {
        return id; // idempotent re-append of the current head
    }
    states_.emplace_back(id, stateBytes);
    head_ = id;
    return id;
}

Bytes MemoryStore::materialize(const ContentId& id) const
{
    for (const auto& [key, stateBytes] : states_)
    {
        if (key == id)
        {
            return stateBytes;
        }
    }
    return {};
}

Bytes MemoryStore::readDelta(const ContentId& base, const ContentId& target) const
{
    static_cast<void>(base);    // state-replication: the receiver materializes the
    return materialize(target); // target; a real transport encodes the diff
}

bool MemoryStore::verify(const ContentId& id) const
{
    for (const auto& [key, stateBytes] : states_)
    {
        static_cast<void>(stateBytes);
        if (key == id)
        {
            return true;
        }
    }
    return false;
}

ContentId MemoryStore::head() const
{
    return head_;
}

// --- GitStore: documented sketch, deliberately unimplemented -----------------

ContentId GitStore::append(const Bytes& stateBytes, const ContentId& base)
{
    static_cast<void>(stateBytes);
    static_cast<void>(base);
    throw std::logic_error("qiven-context-draft: git transport is a documented sketch");
}

Bytes GitStore::materialize(const ContentId& id) const
{
    static_cast<void>(id);
    throw std::logic_error("qiven-context-draft: git transport is a documented sketch");
}

Bytes GitStore::readDelta(const ContentId& base, const ContentId& target) const
{
    static_cast<void>(base);
    static_cast<void>(target);
    throw std::logic_error("qiven-context-draft: git transport is a documented sketch");
}

bool GitStore::verify(const ContentId& id) const
{
    static_cast<void>(id);
    throw std::logic_error("qiven-context-draft: git transport is a documented sketch");
}

ContentId GitStore::head() const
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
std::optional<ExecutionGrant> QivenContext::g_activeGrant;
AuthenticatedActor QivenContext::g_grantActor;

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
    QuarantineState quarantine { QuarantineState::Promoted };
    switch (source.kind)
    {
    case CognitionSourceKind::CanonicalRemote:
    {
        const ContentId id = source.contentId.empty() ? g_store->head() : source.contentId;
        bytes              = g_store->materialize(id);
        if (bytes.empty())
        {
            // genesis: no canonical state yet — materialize the empty cognition
            // (governance + constitution + default policy) and append it as state #1
            bytes                    = serializeImpl(makeGenesis());
            const ContentId appended = g_store->append(bytes, {});
            QIVEN_ASSERT(!appended.empty());
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

    const Epoch epoch = g_epoch.fetch_add(1) + 1; // fencing token per materialization
    auto live         = std::make_shared<Materialization>();
    live->state       = std::make_shared<const Snapshot>(std::move(parsed.snapshot));
    live->contentId   = DraftContentId(bytes); // content identity of this very state
    live->epoch       = epoch;
    live->quarantine  = quarantine;

    g_live.push_back(Registry { live });
    return CognitionHandle { live }; // participants get the const view
}

Data QivenContext::ReadFromCognition(const CognitionHandle& handle, const Query& query)
{
    static_cast<void>(query); // typed Bundle with floors lands in Phase 2 (DR-007)
    QIVEN_ASSERT(handle != nullptr);
    // no admission lock: the snapshot is a by-value copy of immutable semantics
    return serializeImpl(*handle->state);
}

std::optional<ExecutionGrant> QivenContext::AcquireGrant(const AuthenticatedActor& actor, WorkMode mode)
{
    std::lock_guard lock(g_admission);
    if (g_activeGrant.has_value())
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
    const ExecutionGrant grant { g_epoch.load(), actor.principal, mode };
    g_activeGrant = grant;
    g_grantActor  = actor;
    return grant;
}

void QivenContext::ReleaseGrant(const ExecutionGrant& grant)
{
    std::lock_guard lock(g_admission);
    if (g_activeGrant && *g_activeGrant == grant)
    {
        g_activeGrant.reset();
        g_grantActor = AuthenticatedActor {};
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

    Registry* entry = nullptr; // gate 0: known handle, non-quarantined
    for (auto& live : g_live)
    {
        if (live.live == handle)
        {
            entry = &live;
            break;
        }
    }
    if (entry == nullptr || entry->live->quarantine != QuarantineState::Promoted)
    {
        return refuse(RefusalReason::GovernanceDenied); // pit P-33: restore never self-promotes
    }
    if (!g_activeGrant || !(*g_activeGrant == grant))
    {
        return refuse(RefusalReason::GrantRefused); // stale or foreign grants never regain authority
    }
    const auto governance = CanonicalHeadLocked(); // context passed IN — no re-entry (P-42)
    if (!governance || !g_identity || !g_identity->verify(g_grantActor, *governance))
    {
        return refuse(RefusalReason::UnverifiedActor); // re-checked at commit (ADR-0033 §4)
    }
    if (grant.mode == WorkMode::Unattended && !transaction.operations.empty())
    {
        return refuse(RefusalReason::UnattendedMutation); // ADR-0036: read-only default
    }
    if (transaction.base != entry->live->contentId)
    {
        return refuse(RefusalReason::StaleBase); // durable divergence: re-read (P-25 family)
    }

    if (transaction.operations.empty())
    {
        return Verdict { Verdict::Outcome::Applied, RefusalReason::StoreDiverged,
                         entry->live->contentId }; // ordinary turn: nothing to store
    }

    // gates 5-6: policy table + content-bound H2 + invariants, for the WHOLE
    // transaction before anything applies (no partial snapshot, ADR-0033 §4)
    for (const auto& operation : transaction.operations)
    {
        OperationClass opClass = OperationClass::StateUpdate;
        switch (operation.kind)
        {
        case Operation::Kind::AppendDecision: opClass = OperationClass::DecisionAcceptance; break;
        case Operation::Kind::AddMemory: opClass = OperationClass::MemoryWrite; break;
        case Operation::Kind::UpsertObligation:
        case Operation::Kind::CloseObligation: opClass = OperationClass::ObligationWrite; break;
        case Operation::Kind::UpdateState: opClass = OperationClass::StateUpdate; break;
        }
        const HandoffPolicy* row = nullptr;
        for (const auto& candidate : entry->live->state->policy.handoff)
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
            if (transaction.h2->reviewer.empty() || transaction.handoffEvidenceRef.empty())
            {
                return refuse(RefusalReason::HandoffMissing);
            }
            if (transaction.h2->reviewedDeltaDigest != DigestOperations(transaction.operations))
            {
                return refuse(RefusalReason::HandoffInvalid); // evidence not bound to THIS delta
            }
        }
    }

    const Snapshot& current = *entry->live->state;
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
        case Operation::Kind::AddMemory:
            if (operation.payload.empty())
            {
                return refuse(RefusalReason::InvariantFailed);
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
        case Operation::Kind::UpdateState:
            if (operation.payload.empty())
            {
                return refuse(RefusalReason::InvariantFailed);
            }
            break;
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
        case Operation::Kind::AddMemory:
            next.memory.push_back(MemoryRecord { MemoryRecord::Kind::Lesson,
                                                 MemoryRecord::Status::Active,
                                                 operation.title,
                                                 operation.payload,
                                                 Provenance {} });
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
        case Operation::Kind::UpdateState:
            next.state.current = operation.payload;
            break;
        }
    }

    const Bytes newState = serializeImpl(next);
    const ContentId id   = g_store->append(newState, entry->live->contentId);
    if (id.empty())
    {
        return refuse(RefusalReason::StoreDiverged); // store-level CAS failed: fail closed
    }
    entry->live->state     = std::make_shared<const Snapshot>(std::move(next));
    entry->live->contentId = id; // the durable fencing token advances
    return Verdict { Verdict::Outcome::Applied, RefusalReason::StoreDiverged, id };
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
    if (head.empty())
    {
        return nullptr;
    }
    for (const auto& entry : g_live)
    {
        if (entry.live->contentId == head && entry.live->quarantine == QuarantineState::Promoted)
        {
            return entry.live->state; // fast path: the live canonical materialization
        }
    }
    const auto parsed = deserializeImpl(g_store->materialize(head));
    if (!parsed.ok)
    {
        return nullptr;
    }
    return std::make_shared<const Snapshot>(std::move(parsed.snapshot));
}

bool QivenContext::RetireCognition(const CognitionHandle& handle)
{
    std::lock_guard lock(g_admission);
    for (std::size_t i = 0; i < g_live.size(); ++i)
    {
        if (g_live[i].live == handle)
        {
            g_live.erase(g_live.begin() + static_cast<std::ptrdiff_t>(i));
            return true; // destruction deferred until the last pin releases
        }
    }
    return false;
}

bool QivenContext::IsContinueable(const Human* human, const LLMClientTool* client,
                                  const Device* device, const LLM* llm,
                                  const CognitionHandle& handle)
{
    if (human == nullptr || client == nullptr || device == nullptr || llm == nullptr || !handle)
    {
        return false;
    }
    if (g_store == nullptr || !g_store->verify(handle->contentId))
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
