#if !defined(TEBTRO_GENERAL_H)


//
// @note types
//

#if !defined(local_persist)
#    define local_persist static
#endif
#if !defined(inline)
#    define inline static
#endif

//
// @note defer
//

#define CONCAT_INTERNAL(x,y) x ## y
#define CONCAT(x,y) CONCAT_INTERNAL(x,y)

template<typename T>
struct Exit_Scope {
    T lambda;
    Exit_Scope(T lambda): lambda(lambda) {}
    ~Exit_Scope() {
        lambda();
    }
    Exit_Scope(const Exit_Scope&);
    private:
    Exit_Scope& operator = (const Exit_Scope&);
};

struct Exit_Scope_Help {
    template<typename T>
        Exit_Scope<T> operator+(T t) {
        return t;
    }
};

#define defer const auto& CONCAT(defer__, __LINE__) = Exit_Scope_Help() + [&]()


// @note Disable compiler warning: local variable is initialized but not referenced
#pragma warning(disable : 4189)


#define TEBTRO_GENERAL_H
#endif // TEBTRO_GENERAL_H
