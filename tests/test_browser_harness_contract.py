from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TESTS = ROOT / "tests"


def test_browser_tests_use_sandbox_safe_shared_runtime():
    runtime = TESTS / "browser_test_runtime.js"
    assert runtime.is_file(), "shared browser runtime is required"

    runtime_source = runtime.read_text()
    assert "chromium.launch" in runtime_source
    assert "--single-process" in runtime_source
    assert "--no-zygote" in runtime_source
    assert "NetworkServiceInProcess" in runtime_source
    assert "/Applications/Google Chrome.app" not in runtime_source

    for name in ("test_dashboard_2027_browser.js", "test_nag_layout_browser.js"):
        source = (TESTS / name).read_text()
        assert "require('./browser_test_runtime')" in source
        assert "/Applications/Google Chrome.app" not in source

    dashboard_source = (TESTS / "test_dashboard_2027_browser.js").read_text()
    assert "require('node:http')" not in dashboard_source
    assert "server.listen(" not in dashboard_source
    assert "page.route(" in dashboard_source


if __name__ == "__main__":
    test_browser_tests_use_sandbox_safe_shared_runtime()
    print("sandbox-safe browser harness contract: PASS")
