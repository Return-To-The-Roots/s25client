# Checks for some C++ features and sets CXX_HAVE_{feature}
#
# Currently:
#     constexpr
#     override

INCLUDE(CheckCXXSourceCompiles)

UNSET(CXX_HAVE_CONSTEXPR CACHE)
CHECK_CXX_SOURCE_COMPILES("
  constexpr int foo(){ return 0; }

  template<int v>
  struct A{
    static constexpr int value = v;
  };

  int main(void){
    return A<foo()>::value;
  }
" CXX_HAVE_CONSTEXPR
)

UNSET(CXX_HAVE_OVERRIDE CACHE)
CHECK_CXX_SOURCE_COMPILES("
	class Base{
	public:
		virtual int foo(int bar){ return bar; }
	};

	class Sub : public Base {
	public:
		virtual int foo(int bar) override{ return 2*bar; };
	};

	int main()
	{
		Base b;
		Sub s;

		return (b.foo(2) * 2 == s.foo(2)) ? 0 : 1;
	}
" CXX_HAVE_OVERRIDE
)