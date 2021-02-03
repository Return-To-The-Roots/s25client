# Code coverage

We use code coverage to make help writing good tests.

In general all new features should be covered by a test and all bugfixes should include a test that reproduces the bug.
The CI will notify about reduced coverage but a failing check will not block a PR.
Hence you are encouraged to make sure your contributions are well tested but we acknowledge that for some things this is very hard to achieve.

For test code we require 100% line coverage.
This is due to the observation that an uncovered line in test code is almost always a mistake or unexpected.
In rare circumstances test code is really unreachable.
For example output operators are only called on a failing test or defensive code checking that something does not happen can also not reasonably be reached.
For those you can use markers:

```c++
// LCOV_EXCL_START
std::ostream& operator<<(std::ostream& os, Type t)
{
    ...
}
// LCOV_EXCL_STOP

for(....)
  BOOST_TEST_FAIL("Should not be reached"); // LCOV_EXCL_LINE
```

To verify this locally build with `RTTR_ENABLE_COVERAGE` and run the tests.
Then you can use the helpers script `tools/ci/collectCoverageData.sh` which will download `lcov` and run it on the measured data to produce a `coverage.info` file with all relevant coverage info.
