// ============================================================================
// persistence.cpp — serialization, memory store, and the gated service
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
constexpr std::uint8_t kSerializationVersion = 1;

// --- little-endian TLV helpers (fixed field order, versioned) ----------------

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

std::uint8_t getU8(const Bytes& bytes, std::size_t& offset)
{
    QIVEN_ASSERT(offset < bytes.size());
    return std::to_integer<std::uint8_t>(bytes[offset++]);
}

std::uint32_t getU32(const Bytes& bytes, std::size_t& offset)
{
    QIVEN_ASSERT(offset + 4 <= bytes.size());
    std::uint32_t value = 0;
    for (unsigned shift = 0; shift < 32; shift += 8)
    {
        value |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset++])) << shift;
    }
    return value;
}

std::uint64_t getU64(const Bytes& bytes, std::size_t& offset)
{
    QIVEN_ASSERT(offset + 8 <= bytes.size());
    std::uint64_t value = 0;
    for (unsigned shift = 0; shift < 64; shift += 8)
    {
        value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset++])) << shift;
    }
    return value;
}

std::int64_t getI64(const Bytes& bytes, std::size_t& offset)
{
    const std::uint64_t bits = getU64(bytes, offset);
    std::int64_t value       = 0;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

std::string getStr(const Bytes& bytes, std::size_t& offset)
{
    const auto length = getU32(bytes, offset);
    QIVEN_ASSERT(offset + length <= bytes.size());
    std::string value;
    value.resize(length);
    if (length > 0)
    {
        std::memcpy(value.data(), bytes.data() + offset, length);
    }
    offset += length;
    return value;
}

// serialization: versioned fixed-order projection of the value tree.
// epoch and contentId are deliberately NOT serialized: epoch is minted per
// materialization, contentId is the digest of these very bytes.
Bytes serializeImpl(const LLMCognition& cognition)
{
    Bytes bytes;
    putU8(bytes, kSerializationVersion);

    putStr(bytes, cognition.governance.rootPrincipal);
    putU32(bytes, static_cast<std::uint32_t>(cognition.constitution.articles.size()));
    for (const auto& article : cognition.constitution.articles)
    {
        putStr(bytes, article);
    }

    putU32(bytes, static_cast<std::uint32_t>(cognition.collaborations.size()));
    for (const auto& rule : cognition.collaborations)
    {
        putU8(bytes, static_cast<std::uint8_t>(rule.domain));
        putStr(bytes, rule.rule);
    }

    putStr(bytes, cognition.state.activeWork);
    putStr(bytes, cognition.state.current);
    putStr(bytes, cognition.state.repositories);
    putStr(bytes, cognition.state.roadmap);

    putU32(bytes, static_cast<std::uint32_t>(cognition.decisions.size()));
    for (const auto& decision : cognition.decisions)
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

    putU32(bytes, static_cast<std::uint32_t>(cognition.memory.size()));
    for (const auto& record : cognition.memory)
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

    putU32(bytes, static_cast<std::uint32_t>(cognition.obligations.size()));
    for (const auto& obligation : cognition.obligations)
    {
        putU8(bytes, static_cast<std::uint8_t>(obligation.status));
        putU8(bytes, static_cast<std::uint8_t>(obligation.trigger));
        putI64(bytes, obligation.id);
        putStr(bytes, obligation.statement);
        putStr(bytes, obligation.completionCriteria);
    }

    putU32(bytes, static_cast<std::uint32_t>(cognition.evidence.size()));
    for (const auto& evidence : cognition.evidence)
    {
        putStr(bytes, evidence.boundTo);
        putStr(bytes, evidence.content);
    }

    return bytes;
}

LLMCognition deserializeImpl(const Bytes& bytes)
{
    std::size_t offset = 0;
    QIVEN_ASSERT(!bytes.empty());
    // the version read has a side effect (offset advance) - it must NEVER live
    // inside QIVEN_ASSERT, which compiles to nothing in Release (NDEBUG)
    const std::uint8_t version = getU8(bytes, offset);
    QIVEN_ASSERT(version == kSerializationVersion);

    LLMCognition cognition { 0, {}, {}, {}, {}, {}, {}, {}, {}, {} };
    cognition.governance.rootPrincipal = getStr(bytes, offset);

    const auto articleCount = getU32(bytes, offset);
    cognition.constitution.articles.reserve(articleCount);
    for (std::uint32_t i = 0; i < articleCount; ++i)
    {
        cognition.constitution.articles.push_back(getStr(bytes, offset));
    }

    const auto collaborationCount = getU32(bytes, offset);
    cognition.collaborations.reserve(collaborationCount);
    for (std::uint32_t i = 0; i < collaborationCount; ++i)
    {
        CollaborationRule rule;
        rule.domain = static_cast<CollaborationRule::Domain>(getU8(bytes, offset));
        rule.rule   = getStr(bytes, offset);
        cognition.collaborations.push_back(std::move(rule));
    }

    cognition.state.activeWork   = getStr(bytes, offset);
    cognition.state.current      = getStr(bytes, offset);
    cognition.state.repositories = getStr(bytes, offset);
    cognition.state.roadmap      = getStr(bytes, offset);

    const auto decisionCount = getU32(bytes, offset);
    cognition.decisions.reserve(decisionCount);
    for (std::uint32_t i = 0; i < decisionCount; ++i)
    {
        Decision decision;
        decision.id                = getI64(bytes, offset);
        decision.status            = static_cast<Lifecycle>(getU8(bytes, offset));
        decision.title             = getStr(bytes, offset);
        decision.content           = getStr(bytes, offset);
        const auto supersedesCount = getU32(bytes, offset);
        decision.supersedes.reserve(supersedesCount);
        for (std::uint32_t k = 0; k < supersedesCount; ++k)
        {
            decision.supersedes.push_back(getI64(bytes, offset));
        }
        const auto supersededByCount = getU32(bytes, offset);
        decision.supersededBy.reserve(supersededByCount);
        for (std::uint32_t k = 0; k < supersededByCount; ++k)
        {
            decision.supersededBy.push_back(getI64(bytes, offset));
        }
        const auto sourceCount = getU32(bytes, offset);
        decision.provenance.sources.reserve(sourceCount);
        for (std::uint32_t k = 0; k < sourceCount; ++k)
        {
            decision.provenance.sources.push_back(getStr(bytes, offset));
        }
        cognition.decisions.push_back(std::move(decision));
    }

    const auto memoryCount = getU32(bytes, offset);
    cognition.memory.reserve(memoryCount);
    for (std::uint32_t i = 0; i < memoryCount; ++i)
    {
        MemoryRecord record;
        record.kind            = static_cast<MemoryRecord::Kind>(getU8(bytes, offset));
        record.status          = static_cast<MemoryRecord::Status>(getU8(bytes, offset));
        record.title           = getStr(bytes, offset);
        record.statement       = getStr(bytes, offset);
        const auto sourceCount = getU32(bytes, offset);
        record.provenance.sources.reserve(sourceCount);
        for (std::uint32_t k = 0; k < sourceCount; ++k)
        {
            record.provenance.sources.push_back(getStr(bytes, offset));
        }
        cognition.memory.push_back(std::move(record));
    }

    const auto obligationCount = getU32(bytes, offset);
    cognition.obligations.reserve(obligationCount);
    for (std::uint32_t i = 0; i < obligationCount; ++i)
    {
        Obligation obligation;
        obligation.status             = static_cast<Obligation::Status>(getU8(bytes, offset));
        obligation.trigger            = static_cast<Obligation::TriggerKind>(getU8(bytes, offset));
        obligation.id                 = getI64(bytes, offset);
        obligation.statement          = getStr(bytes, offset);
        obligation.completionCriteria = getStr(bytes, offset);
        cognition.obligations.push_back(std::move(obligation));
    }

    const auto evidenceCount = getU32(bytes, offset);
    cognition.evidence.reserve(evidenceCount);
    for (std::uint32_t i = 0; i < evidenceCount; ++i)
    {
        EvidenceRecord evidence;
        evidence.boundTo = getStr(bytes, offset);
        evidence.content = getStr(bytes, offset);
        cognition.evidence.push_back(std::move(evidence));
    }

    return cognition;
}

Constitution makeConstitution()
{
    Constitution constitution; // the 18 article titles; full text stays canonical
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

// --- MemoryStore: the contract, proven --------------------------------------

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

// --- QivenContext: the gated service -----------------------------------------

std::mutex QivenContext::g_admission;
std::atomic<Epoch> QivenContext::g_epoch { 0 };
std::shared_ptr<ICognitionStore> QivenContext::g_store;
std::vector<QivenContext::Registry> QivenContext::g_live;

void QivenContext::attachStore(std::shared_ptr<ICognitionStore> store)
{
    std::lock_guard lock(g_admission);
    g_store = std::move(store);
}

std::shared_ptr<LLMCognition> QivenContext::CreateCognition(const CognitionSource& source)
{
    std::lock_guard lock(g_admission);
    QIVEN_ASSERT(g_store != nullptr); // draft precondition: attachStore before boot

    Bytes bytes;
    bool quarantined = false;
    switch (source.kind)
    {
    case CognitionSourceKind::CanonicalRemote:
    {
        const ContentId id = source.contentId.empty() ? g_store->head() : source.contentId;
        bytes              = g_store->materialize(id);
        if (bytes.empty())
        {
            // genesis: no canonical state yet — materialize the empty cognition
            // (governance defaults + constitution) and append it as state #1
            const LLMCognition genesis { 0,
                                         {},
                                         Governance { "github:JasonHuang3D" },
                                         makeConstitution(),
                                         {},
                                         State {},
                                         {},
                                         {},
                                         {},
                                         {} };
            bytes                    = serializeImpl(genesis);
            const ContentId appended = g_store->append(bytes, {});
            QIVEN_ASSERT(!appended.empty());
        }
        break;
    }
    case CognitionSourceKind::HandoffArtifact:
        bytes       = source.inlineBytes; // K4: restored from artifact bytes alone
        quarantined = true;               // non-authoritative until a governed cutover
        break;
    case CognitionSourceKind::CompressedStream:
        throw std::logic_error("qiven-context-draft: K5 compressed transport not implemented");
    }
    QIVEN_ASSERT(!bytes.empty());

    LLMCognition restored = deserializeImpl(bytes);
    const Epoch epoch     = g_epoch.fetch_add(1) + 1; // authority token per materialization
    auto cognition        = std::shared_ptr<LLMCognition>(
        new LLMCognition(epoch,
                                DraftContentId(bytes), // content identity of this very state
                                std::move(restored.governance),
                                std::move(restored.constitution),
                                std::move(restored.collaborations),
                                std::move(restored.state),
                                std::move(restored.decisions),
                                std::move(restored.memory),
                                std::move(restored.obligations),
                                std::move(restored.evidence)));

    g_live.push_back(Registry { cognition, quarantined });
    return cognition;
}

Data QivenContext::ReadFromCognition(const LLMCognition* cognition, const Query& query)
{
    static_cast<void>(query); // budget enforcement is K5's domain (lossless transport)
    // no admission lock: the snapshot is a by-value copy of immutable semantics
    return serializeImpl(*cognition);
}

bool QivenContext::WriteToCognition(LLMCognition* cognition, const TransactionDelta& delta,
                                    WorkMode mode)
{
    std::lock_guard lock(g_admission);

    Registry* entry = nullptr; // 0. known, current, non-quarantined
    for (auto& live : g_live)
    {
        if (live.cognition.get() == cognition)
        {
            entry = &live;
            break;
        }
    }
    if (entry == nullptr || entry->quarantined)
    {
        return false;
    }
    if (cognition->epoch != g_epoch.load())
    {
        return false; // runtime swap happened: think again (authority)
    }
    if (delta.kind == TransactionDelta::Kind::None)
    {
        return true; // ordinary turn: no context commit needed
    }
    if (delta.base != cognition->contentId)
    {
        return false; // durable divergence: re-read (authority)
    }

    if (delta.kind == TransactionDelta::Kind::AppendDecision)
    { // 2. typed handoff
        if (!delta.handoff.has_value() || delta.handoff != Handoff::H2_Review || delta.handoffEvidenceRef.empty())
        {
            return false; // acceptance = merge-class semantics; waiver N/A
        }
    }

    if (mode == WorkMode::Unattended)
    {
        return false; // 3. unattended = read-only default (ADR-0036)
    }

    switch (delta.kind)
    { // 4. record invariants (draft subset)
    case TransactionDelta::Kind::AppendDecision:
        if (delta.recordId <= 0 || delta.title.empty() || delta.payload.empty())
        {
            return false;
        }
        for (const auto& existing : cognition->decisions)
        {
            if (existing.id == delta.recordId)
            {
                return false; // ids are never reused
            }
        }
        break;
    case TransactionDelta::Kind::AddMemoryRecord:
    case TransactionDelta::Kind::UpsertObligation:
        if (delta.payload.empty() || (delta.kind == TransactionDelta::Kind::UpsertObligation && delta.recordId <= 0))
        {
            return false;
        }
        break;
    case TransactionDelta::Kind::CloseObligation:
    {
        bool foundOpen = false;
        for (const auto& obligation : cognition->obligations)
        {
            if (obligation.id == delta.recordId && obligation.status == Obligation::Status::Open)
            {
                foundOpen = true;
                break;
            }
        }
        if (!foundOpen)
        {
            return false;
        }
        break;
    }
    case TransactionDelta::Kind::UpdateState:
        if (delta.payload.empty())
        {
            return false;
        }
        break;
    case TransactionDelta::Kind::None:
        return false; // handled above; defensive
    }

    switch (delta.kind)
    { // 5. apply, then persist the new full state
    case TransactionDelta::Kind::AppendDecision:
        cognition->decisions.push_back(Decision { delta.recordId,
                                                  Lifecycle::Accepted,
                                                  delta.title,
                                                  delta.payload,
                                                  {},
                                                  {},
                                                  Provenance { { delta.handoffEvidenceRef } } });
        break;
    case TransactionDelta::Kind::AddMemoryRecord:
        cognition->memory.push_back(MemoryRecord { MemoryRecord::Kind::Lesson,
                                                   MemoryRecord::Status::Active,
                                                   delta.title,
                                                   delta.payload,
                                                   Provenance {} });
        break;
    case TransactionDelta::Kind::UpsertObligation:
        cognition->obligations.push_back(Obligation { Obligation::Status::Open,
                                                      Obligation::TriggerKind::Manual,
                                                      delta.recordId,
                                                      delta.payload,
                                                      {} });
        break;
    case TransactionDelta::Kind::CloseObligation:
        for (auto& obligation : cognition->obligations)
        {
            if (obligation.id == delta.recordId)
            {
                obligation.status = Obligation::Status::Done; // history kept, status moves
                break;
            }
        }
        break;
    case TransactionDelta::Kind::UpdateState:
        cognition->state.current = delta.payload;
        break;
    case TransactionDelta::Kind::None:
        break;
    }

    const Bytes newState = serializeImpl(*cognition);
    const ContentId id   = g_store->append(newState, cognition->contentId);
    if (id.empty())
    {
        return false; // store-level divergence (paranoid double-check)
    }
    cognition->contentId = id; // the durable fencing token advances
    return true;
}

Epoch QivenContext::currentEpoch()
{
    return g_epoch.load(); // lock-free sampling: cheap staleness pre-checks
}

bool QivenContext::RetireCognition(const std::shared_ptr<LLMCognition>& cognition)
{
    std::lock_guard lock(g_admission);
    for (std::size_t i = 0; i < g_live.size(); ++i)
    {
        if (g_live[i].cognition == cognition)
        {
            g_live.erase(g_live.begin() + static_cast<std::ptrdiff_t>(i));
            return true; // destruction deferred until the last pin releases
        }
    }
    return false;
}

bool QivenContext::IsContinueable(const Human* human, const LLMClientTool* client,
                                  const Device* device, const LLM* llm,
                                  const LLMCognition* cognition)
{
    if (human == nullptr || client == nullptr || device == nullptr || llm == nullptr || cognition == nullptr)
    {
        return false;
    }
    if (cognition->epoch != g_epoch.load())
    {
        return false; // stale materialization
    }
    if (g_store == nullptr || !g_store->verify(cognition->contentId))
    {
        return false; // state must be restorable from the store
    }
    if (human->verifiedPrincipal.empty() || human->verifiedPrincipal != cognition->governance.rootPrincipal)
    {
        return false; // authority present (R4)
    }
    if (!client->operational || !device->reachable)
    {
        return false; // runtime plane alive
    }
    for (const auto& obligation : cognition->obligations)
    {
        if (obligation.status == Obligation::Status::Blocked)
        {
            return false; // an explicit blocker halts the loop
        }
    }
    return true; // budget remains: open design question (draft)
}
} // namespace qiven::context
