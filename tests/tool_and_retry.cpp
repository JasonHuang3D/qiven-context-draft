// ============================================================================
// tool_and_retry — v4.4 compiled proof (roadmap v4.4; seed §17/§18/§19):
//   A6: a contract-bearing invocation is constructed mechanically and a
//       guessed variant FAILS validation (syntax is mechanism-owned)
//   A7: failure normalization strips volatile text; a fingerprint recalls
//       related pit records; an equivalent retry without new evidence is
//       REFUSED (V4-R3)
// ============================================================================

#include <qiven/context/context.hpp>
#include <qiven/context/persistence.hpp>
#include <qiven/context/runtime.hpp>

#include <cstdio>
#include <string>
#include <vector>

#include "detail/check.hpp"

using namespace qiven::context;

// pit.tool_contract_advertises_only_implemented_behavior (V4S-05/S1-04)

int main()
{
    // --- A6: the contract constructs; the guess fails -----------------------
    {
        ToolContract contract; // the declared qiven gate surface
        contract.tool         = "qiven";
        contract.operation    = "gate";
        contract.argvTemplate = { "python", "tools/qiven.py", "gate", "{subject}" };

        ActionIntent intent;
        intent.kind      = ActionKind::InvokeTool;
        intent.tool      = "qiven";
        intent.operation = "gate";
        intent.concepts  = { "local" };

        const auto constructed = construct_invocation(contract, intent);
        QCD_CHECK(constructed.size() == 4);
        QCD_CHECK(constructed[0] == "python" && constructed[1] == "tools/qiven.py");
        QCD_CHECK(constructed[2] == "gate" && constructed[3] == "local");
        QCD_CHECK(validate_invocation(contract, constructed));

        // the A6 failure class: guessed variants the contract never declared
        const std::vector<std::string> guessedTool = { "qiven", "gate", "local" };
        QCD_CHECK(!validate_invocation(contract, guessedTool)); // wrong tool layout
        const std::vector<std::string> guessedFlag = { "python", "tools/qiven.py",
                                                       "gate", "local", "--versbose" };
        QCD_CHECK(!validate_invocation(contract, guessedFlag)); // arity + typo flag
        const std::vector<std::string> guessedName = { "python", "tools/qiven.py",
                                                       "run-gate", "local" };
        QCD_CHECK(!validate_invocation(contract, guessedName)); // fixed word differs
    }

    // --- A7: normalization, pit recall, and the retry rule ------------------
    {
        QCD_CHECK(normalize_failure_text("exit 1 at 20260920T141500Z in C:/tmp/x.log")
                      .find("20260920T141500Z") == std::string::npos);
        QCD_CHECK(normalize_failure_text("gate local failed 20260920T141500Z")
                      .find("gate local failed") != std::string::npos);

        Snapshot snapshot;
        MemoryRecord pit; // a recorded scar that must be recalled before retry
        pit.kind      = MemoryRecord::Kind::Lesson;
        pit.title     = "qiven gate odyssey (A7)";
        pit.statement = "one-assert-per-cycle cost a full gate each round; batch evidence";
        snapshot.memory.push_back(pit);

        FailureFingerprint prior;
        prior.tool          = "qiven";
        prior.operation     = "gate";
        prior.category      = "gate-cycle";
        prior.stableMessage = "gate local failed 20260920T141500Z";

        const auto related = find_related_records(snapshot, prior);
        QCD_CHECK(!related.empty()); // the known pit IS recalled
        QCD_CHECK(related.front().find("odyssey") != std::string::npos);

        ActionIntent retry = []() {
            ActionIntent intent;
            intent.kind      = ActionKind::RetryFailure;
            intent.tool      = "qiven";
            intent.operation = "gate";
            FailureFingerprint f;
            f.stableMessage     = "gate local failed 20260921T090000Z"; // later timestamp:
            intent.priorFailure = f;                                    // same signature
            return intent;
        }();
        QCD_CHECK(!retry_permitted(prior, retry, RetryEvidenceState {}));      // REFUSED
        QCD_CHECK(retry_permitted(prior, retry, RetryEvidenceState { true })); // permitted

        retry.tool = "git"; // a different tool is not an equivalent retry
        QCD_CHECK(retry_permitted(prior, retry, RetryEvidenceState {}));
    }

    // --- T1 / pit.empty_failure_selector_never_matches: an empty selector
    // is NOT a wildcard - std::string::find("") matches every string ----
    {
        Snapshot snapshot;
        MemoryRecord unrelated;
        unrelated.kind      = MemoryRecord::Kind::Lesson;
        unrelated.title     = "unrelated compiler lesson";
        unrelated.statement = "completely unrelated";
        snapshot.memory.push_back(unrelated);
        FailureFingerprint empty;
        empty.tool     = "";
        empty.category = "";
        QCD_CHECK(find_related_records(snapshot, empty).empty()); // no false hit
    }

    // --- T2: control stays BLOCKED on an under-specified fingerprint ------
    {
        Snapshot snapshot;
        snapshot.invocation = default_invocation_policy();
        MemoryRecord unrelated;
        unrelated.kind      = MemoryRecord::Kind::Risk;
        unrelated.title     = "unrelated disk risk";
        unrelated.statement = "nothing to do with any tool here";
        snapshot.memory.push_back(unrelated);
        ActionIntent retry;
        retry.kind      = ActionKind::RetryFailure;
        retry.tool      = "git";
        retry.operation = "status";
        FailureFingerprint empty;
        empty.tool                     = ""; // under-specified: selectors empty
        empty.category                 = "";
        retry.priorFailure             = empty;
        const PreparationPacket packet = build_preparation_packet(snapshot, retry);
        QCD_CHECK(packet.knownPits.empty());     // no false recall
        QCD_CHECK(!packet.ready_for_judgment()); // InspectKnownPit Failed
        QCD_CHECK(packet.failure == PreparationFailure::RequiredRecallMissing);
    }

    // --- T3: positive control - legitimate pit recall still works ---------
    {
        Snapshot scarred;
        scarred.invocation = default_invocation_policy();
        MemoryRecord scar;
        scar.kind      = MemoryRecord::Kind::Lesson;
        scar.title     = "qiven gate odyssey";
        scar.statement = "one-assert-per-cycle cost a full gate each round";
        scarred.memory.push_back(scar);
        ActionIntent retry;
        retry.kind = ActionKind::RetryFailure;
        FailureFingerprint fp;
        fp.tool                        = "qiven";
        fp.stableMessage               = "gate local failed";
        retry.priorFailure             = fp;
        const PreparationPacket packet = build_preparation_packet(scarred, retry);
        QCD_CHECK(!packet.knownPits.empty());   // the pit IS recalled
        QCD_CHECK(packet.ready_for_judgment()); // requirement Satisfied
    }

    std::printf("[ OK ] tool contracts + retry discipline: A6 guess fails, A7 blind retry refused\n");
    return 0;
}
