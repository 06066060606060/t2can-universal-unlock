"""A compiler failure in an extracted C++ Python test must be setup, not RED."""
from continuous_ap_host_fixture import run
try:
 run('int main(){ static_assert(false, "fixture setup probe"); }')
except FileNotFoundError as error:
 assert 'compile/setup failure' in str(error)
else:
 raise AssertionError('compiler failure was not classified as setup')
print('Production host fixture compile-error classification PASS')
