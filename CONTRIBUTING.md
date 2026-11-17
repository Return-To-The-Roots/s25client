# Compilation/Formatting

- Read the README.md in the repository root for instructions on how to compile on Linux, Windows/Visual Studio, ...
- Compilation **MUST** finish without any warnings
- Formatting uses clang-format-10 and can be automatically done via `make clangFormat`
- `clang-tidy` is used for further static analysis, `tools/runClangTidy.sh` can be used to run it manually with a configured build tree

# Testing / Bug fixing

- For each Bug fixed/feature added, there should be a test (GUI excluded until this is possible too)
- Keep tests small, complete and descriptive
- Some tests may run differently based on the Random Generator (RNG), make sure they succeed in all cases but keep the constraints tight
- If a test fails suddenly, it may depend on the RNG. Make sure the behaviour is correct before e.g increasing max. GF amount to wait till the event is checked

# Naming

- Classes/Structs: UpperCamelCase
- Parameters, local variables, member functions: lowerCamelCase
- Member variables: Suffix "\_" (e.g. `myMember_`)
- Do not shadow variables (e.g. using the name of a member as a parameter)
- Use imperative for setters and question-style for getters e.g. `lieSomewhere`, `carry`, `burn` and `isLying`, `isValid`, `canChange`

# Locals / String encoding / Paths / UTF8

- All strings (`std::string`) used in the program **must** be encoded in UTF8. Conversion from different encodings must be done on reading/writing ONLY. Use the helper functions in libUtil (`ucString.h` or `utf8.h`)
- Filepaths/Folderpaths must be stored in `bfs::path` variables
- 'Filepath' means a complete path whereas 'Filename' is just the name itself
- Use `bfs` function for all file system operations
- Note: 'bfs::path::string()` returns an UTF8 encoded string already. No need to use conversions there (see https://github.com/Return-To-The-Roots/s25client/pull/645)

# General

- Small include files  
    - Use forward declarations instead of including headers
    - Include what you use (e.g. when using `std::vector`, include `<vector>`)
    - Once class/struct per file, avoid "collections" of structs even when they belong partly together
- Check STL/Boost before rolling your own implementations
- Implementing any of the **Big 3** (copy ctor, copy assignment, destructor) requires implementing the others as well. For classes, that should not be copied (e.g. having open handles) declare copy ctor **AND** copy assignment in private region and *do not* implement it. Put a comment there.
- Comment functions you write at least with a summary in the header. use JavaDoc syntax (Either `///` or `/** foo bar */`)
- Make sure assumptions are met ->   
    - Use `static_assert` to check what is possible during compile-time
    - Use exceptions for enforcing stuff at runtime (e.g. parameter values, switch(enum) defaults)
    - Use `assert`s for general conditions that should be met, but are less critical or performance-wise expensive to check
- Avoid unnecessary branches: No need for an `else` after a `return`. Additional branches require additional indentation which makes (especially) longer functions hard to read.
- Avoid code duplication: If you write the same or very similar thing twice (even in one function), factor it out into a new function. This eases maintainability if one ever needs to change or fix that.
- **Document your functions!**   
    - put at least a one-liner in the header describing what it does
    - For more complex functions include also pre-/post-conditions, special cases, ...
    - If you have problems understanding a function and finally figured it out: Help others and add that to the docu
    - Do not describe trivial things like "this is a copy-ctor/sets foo/gets bar" as this should be obvious from the function name (`FooBar(const FooBar&)` / `void setFoo(foo)` / `Bar getBar()`)

# Safety

- Use RAII idiom
    - Encapsulate all resources once they are acquired
    - Use STL collections (`vector`, `list`, ...)
    - Use smart pointers (`std::unique_ptr`, `boost::intrusive_shared_ptr`, `std::shared_pointer` in that order)
- `delete[]` should never be used. Use C++ structs (e.g. `vector`)
- Use `std::array`instead of plain C-arrays
- Do __NOT__ use magic constants. All constants should be made named consts (or defines). For iterations, use iterators or actual sizes
- Do not use printf/sprint/snprintf,fprintf,... They can easily cause buffer overflows. Use C++ streams instead

