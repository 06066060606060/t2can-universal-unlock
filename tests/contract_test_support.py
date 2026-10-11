"""Run retained unittest classes inside isolated feature-contract methods."""
import unittest


def run_contract_cases(owner, classes, expected):
    suite = unittest.TestSuite(
        unittest.defaultTestLoader.loadTestsFromTestCase(case) for case in classes
    )
    discovered = suite.countTestCases()
    owner.assertGreaterEqual(discovered, expected,
                      "A retained unittest method was lost during consolidation")
    result = unittest.TestResult()
    suite.run(result)
    owner.assertEqual(result.testsRun, discovered,
                      "A retained unittest method was not executed")
    problems = result.errors + result.failures
    if problems:
        owner.fail("\n".join(f"{case.id()}:\n{detail}" for case, detail in problems))
    if result.skipped or result.expectedFailures or result.unexpectedSuccesses:
        owner.fail("Retained contracts must run without skipped or expected failures: "
                   f"{result.skipped!r} {result.expectedFailures!r} {result.unexpectedSuccesses!r}")
