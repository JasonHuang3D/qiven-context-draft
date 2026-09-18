// ============================================================================
// runtime.cpp — the causal Work cycle (numbered steps are normative)
// ============================================================================

#include <qiven/context/runtime.hpp>

#include <utility>

namespace qiven::context
{
void LLM::Work(const std::string& prompt, std::string& result, WorkMode mode)
{
    const auto cog = pCognition;       // pin lifetime for this whole cycle (shared_ptr);
                                       // authority is still fenced by epoch (v2 split)
    if (!cog) {
        result = name + ": no cognition bound";
        return;
    }

    // 1. full snapshot for thinking — token cost is K5's optimization target
    const Query query = queryBuilder ? queryBuilder(prompt) : Query{prompt, 0};
    const Data data = QivenContext::ReadFromCognition(cog.get(), query);
    static_cast<void>(data);           // 2-3. VerifyLiveFacts + Thinking: opaque in this draft

    // 4. CallTools / WaitForTools: device-side effects — no tool runtime in this draft

    // 5. build the write shape; no generator = ordinary turn, no context commit
    TransactionDelta delta;
    if (deltaGenerator) {
        delta = deltaGenerator(prompt, *cog);
    }
    if (delta.kind == TransactionDelta::Kind::None) {
        checkpoint.nextAction = "continue";
        result = name + ": no material cognition this turn";
        return;
    }
    delta.base = cog->contentId;       // durable fencing token: my thinking assumed this state

    // 6. gated write — fencing / handoff / unattended / validation inside the API
    if (QivenContext::WriteToCognition(cog.get(), delta, mode)) {
        checkpoint.acceptedRefs.push_back(cog->contentId);   // post-write: the advanced id
        checkpoint.nextAction = "continue";
        result = name + ": delta applied; head=" + cog->contentId;
        return;
    }

    // 7. rejected deltas are held, never silently lost — a token mismatch means
    // the thinking was stale: hold, re-read, re-think (not: retry the write)
    parkedDeltas.push_back(delta);
    checkpoint.unacceptedCandidates.push_back(delta.base);
    checkpoint.knownGaps = "delta parked: admission gate refused";
    result = name + ": delta parked; head=" + cog->contentId;
}
} // namespace qiven::context
