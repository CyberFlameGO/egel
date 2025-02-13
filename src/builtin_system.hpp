#pragma once

#include <fmt/core.h>
#include <math.h>
#include <stdlib.h>

#include <iostream>
#include <map>
#if __has_include(<fmt/args.h>)
#include <fmt/args.h>
#endif

#include "bytecode.hpp"
#include "runtime.hpp"
#include "utils.hpp"

extern int application_argc;  // XXX: get rid of this
extern char **application_argv;

bool add_overflow(long int a, long int b, long int *c) {
    return __builtin_saddl_overflow(a, b, c);
}

bool add_overflow(long long int a, long long int b, long long int *c) {
    return __builtin_saddll_overflow(a, b, c);
}

bool sub_overflow(long int a, long int b, long int *c) {
    return __builtin_ssubl_overflow(a, b, c);
}

bool sub_overflow(long long int a, long long int b, long long int *c) {
    return __builtin_ssubll_overflow(a, b, c);
}

bool mul_overflow(long int a, long int b, long int *c) {
    return __builtin_smull_overflow(a, b, c);
}

bool mul_overflow(long long int a, long long int b, long long int *c) {
    return __builtin_smulll_overflow(a, b, c);
}

// globals
int application_argc = 0;
char **application_argv = nullptr;

/**
 * Egel's system routines.
 *
 * Basic operators, conversions, and some other.
 **/

//## System::k x y - k combinator
class K : public Binary {
public:
    BINARY_PREAMBLE(VM_SUB_BUILTIN, K, "System", "k");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        return arg0;
    }
};

//## System::id x - identity combinator
class Id : public Unary {
public:
    UNARY_PREAMBLE(VM_SUB_BUILTIN, Id, "System", "id");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        return arg0;
    }
};

//## System:min_int - maximum for integers
class MinInt : public Medadic {
public:
    MEDADIC_PREAMBLE(VM_SUB_BUILTIN, MinInt, "System", "min_int");

    VMObjectPtr apply() const override {
        return machine()->create_integer(std::numeric_limits<vm_int_t>::min());
    }
};

//## System::max_int - maximum for integers
class MaxInt : public Medadic {
public:
    MEDADIC_PREAMBLE(VM_SUB_BUILTIN, MaxInt, "System", "max_int");

    VMObjectPtr apply() const override {
        return machine()->create_integer(std::numeric_limits<vm_int_t>::max());
    }
};

//## System::!- x - monadic minus
class MonMin : public Monadic {
public:
    MONADIC_PREAMBLE(VM_SUB_BUILTIN, MonMin, "System", "!-");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        if (machine()->is_integer(arg0)) {
            auto i = machine()->get_integer(arg0);
            vm_int_t res;
            if (mul_overflow((vm_int_t)-1, i, &res)) {
                throw machine()->bad(this, "overflow");
            } else {
                return machine()->create_integer(res);
            }
        } else if (machine()->is_float(arg0)) {
            auto f = machine()->get_float(arg0);
            return machine()->create_float(-f);
        } else {
            throw machine()->bad_args(this, arg0);
        }
    }
};

//## System::+ x y - addition
class Add : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, Add, "System", "+");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_integer(arg0)) && (machine()->is_integer(arg1))) {
            auto i0 = machine()->get_integer(arg0);
            auto i1 = machine()->get_integer(arg1);
            vm_int_t res;
            if (add_overflow(i0, i1, &res)) {
                throw machine()->bad(this, "overflow");
            } else {
                return machine()->create_integer(res);
            }
        } else if ((machine()->is_float(arg0)) && (machine()->is_float(arg1))) {
            auto f0 = machine()->get_float(arg0);
            auto f1 = machine()->get_float(arg1);
            return machine()->create_float(f0 + f1);
        } else if ((machine()->is_text(arg0)) && (machine()->is_text(arg1))) {
            auto f0 = machine()->get_text(arg0);
            auto f1 = machine()->get_text(arg1);
            return VMObjectText::create(f0 + f1);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::+ x y - substraction
class Min : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, Min, "System", "-");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_integer(arg0)) && (machine()->is_integer(arg1))) {
            auto i0 = machine()->get_integer(arg0);
            auto i1 = machine()->get_integer(arg1);
            vm_int_t res;
            if (sub_overflow(i0, i1, &res)) {
                throw machine()->bad(this, "overflow");
            } else {
                return machine()->create_integer(res);
            }
        } else if ((machine()->is_float(arg0)) && (machine()->is_float(arg1))) {
            auto f0 = machine()->get_float(arg0);
            auto f1 = machine()->get_float(arg1);
            return machine()->create_float(f0 - f1);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::* x y - multiplication
class Mul : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, Mul, "System", "*");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_integer(arg0)) && (machine()->is_integer(arg1))) {
            auto i0 = machine()->get_integer(arg0);
            auto i1 = machine()->get_integer(arg1);
            vm_int_t res;
            if (mul_overflow(i0, i1, &res)) {
                throw machine()->bad(this, "overflow");
            } else {
                return machine()->create_integer(res);
            }
        } else if ((machine()->is_float(arg0)) && (machine()->is_float(arg1))) {
            auto f0 = machine()->get_float(arg0);
            auto f1 = machine()->get_float(arg1);
            return machine()->create_float(f0 * f1);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::/ x y - division
class Div : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, Div, "System", "/");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_integer(arg0)) && (machine()->is_integer(arg1))) {
            auto i0 = machine()->get_integer(arg0);
            auto i1 = machine()->get_integer(arg1);
            if (i1 == 0) {
                throw machine()->bad(this, "divide by zero");
            }
            return machine()->create_integer(i0 / i1);
        } else if ((machine()->is_float(arg0)) && (machine()->is_float(arg1))) {
            auto f0 = machine()->get_float(arg0);
            auto f1 = machine()->get_float(arg1);
            if (f1 == 0.0) {
                throw machine()->bad(this, "divide by zero");
            }
            return machine()->create_float(f0 / f1);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::% x y - modulo
class Mod : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, Mod, "System", "%");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_integer(arg0)) && (machine()->is_integer(arg1))) {
            auto i0 = machine()->get_integer(arg0);
            auto i1 = machine()->get_integer(arg1);
            if (i1 == 0) {
                throw machine()->bad(this, "divide by zero");
            }
            return machine()->create_integer(i0 % i1);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::& x y - bitwise and
class BinAnd : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, BinAnd, "System", "&");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_integer(arg0)) && (machine()->is_integer(arg1))) {
            auto i0 = machine()->get_integer(arg0);
            auto i1 = machine()->get_integer(arg1);
            return machine()->create_integer(i0 & i1);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::$ x y - bitwise or
class BinOr : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, BinOr, "System", "$");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_integer(arg0)) && (machine()->is_integer(arg1))) {
            auto i0 = machine()->get_integer(arg0);
            auto i1 = machine()->get_integer(arg1);
            return machine()->create_integer(i0 | i1);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::^ x y - bitwise xor
class BinXOr : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, BinXOr, "System", "^");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_integer(arg0)) && (machine()->is_integer(arg1))) {
            auto i0 = machine()->get_integer(arg0);
            auto i1 = machine()->get_integer(arg1);
            return machine()->create_integer(i0 ^ i1);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::!~ x - bitwise complement
class BinComplement : public Monadic {
public:
    MONADIC_PREAMBLE(VM_SUB_BUILTIN, BinComplement, "System", "!~");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        if ((machine()->is_integer(arg0))) {
            auto i0 = machine()->get_integer(arg0);
            return machine()->create_integer(~i0);
        } else {
            throw machine()->bad_args(this, arg0);
        }
    }
};

//## System::<< x y - bitwise left shift
class BinLeftShift : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, BinLeftShift, "System", "<<");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_integer(arg0)) && (machine()->is_integer(arg1))) {
            auto i0 = machine()->get_integer(arg0);
            auto i1 = machine()->get_integer(arg1);
            return machine()->create_integer(i0 << i1);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::>> x y - bitwise right shift
class BinRightShift : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, BinRightShift, "System", ">>");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_integer(arg0)) && (machine()->is_integer(arg1))) {
            auto i0 = machine()->get_integer(arg0);
            auto i1 = machine()->get_integer(arg1);
            return machine()->create_integer(i0 >> i1);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::< x y - builtin less
class Less : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, Less, "System", "<");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        CompareVMObjectPtr compare;
        if (compare(arg0, arg1) < 0) {
            return machine()->create_true();
        } else {
            return machine()->create_false();
        }
    }
};

//## System::<= x y - builtin less or equals
class LessEq : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, LessEq, "System", "<=");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        CompareVMObjectPtr compare;
        if (compare(arg0, arg1) <= 0) {
            return machine()->create_true();
        } else {
            return machine()->create_false();
        }
    }
};

//## System::== x y - builtin equality
class Eq : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, Eq, "System", "==");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        CompareVMObjectPtr compare;
        if (compare(arg0, arg1) == 0) {
            return machine()->create_true();
        } else {
            return machine()->create_false();
        }
    }
};

//## System::/= x y - builtin inequality
class NegEq : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, NegEq, "System", "/=");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        CompareVMObjectPtr compare;
        if (compare(arg0, arg1) != 0) {
            return machine()->create_true();
        } else {
            return machine()->create_false();
        }
    }
};

//## System::get field obj - retrieve an object field
class GetField : public Binary {
public:
    BINARY_PREAMBLE(VM_SUB_BUILTIN, GetField, "System", "get");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        static symbol_t object = 0;
        if (object == 0) object = machine()->enter_symbol("System", "object");

        if (machine()->is_array(arg1)) {
            auto ff = machine()->get_array(arg1);
            auto sz = ff.size();
            // check head is an object
            if (sz == 0) machine()->bad(this, "invalid");
            if (ff[0]->symbol() != object) machine()->bad(this, "invalid");
            // search for field
            CompareVMObjectPtr compare;
            unsigned int n;
            for (n = 1; n < sz; n = n + 2) {
                if (compare(arg0, ff[n]) == 0) break;
            }
            // return field
            if ((n + 1) < sz) {
                return ff[n + 1];
            } else {
                throw machine()->bad(this, "invalid");
            }
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::set field val obj - set an object field
class SetField : public Triadic {
public:
    TRIADIC_PREAMBLE(VM_SUB_BUILTIN, SetField, "System", "set");

    VMObjectPtr apply(const VMObjectPtr &arg0, const VMObjectPtr &arg1,
                      const VMObjectPtr &arg2) const override {
        static symbol_t object = 0;
        if (object == 0) object = machine()->enter_symbol("System", "object");

        if (machine()->is_array(arg2)) {
            auto ff = machine()->get_array(arg2);
            auto sz = ff.size();
            // check head is an object
            if (sz == 0) machine()->bad(this, "invalid");
            if (ff[0]->symbol() != object) machine()->bad(this, "invalid");
            // search field
            CompareVMObjectPtr compare;
            unsigned int n;
            for (n = 1; n < sz; n = n + 2) {
                if (compare(arg0, ff[n]) == 0) break;
            }
            // set field
            if ((n + 1) < sz) {
                auto arr = VM_OBJECT_ARRAY_CAST(
                    arg2);  // XXX: clean up this cast once. need destructive
                            // update
                arr->set(n + 1, arg1);
                return arg0;
            } else {
                throw machine()->bad(this, "invalid");
            }
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::extend obj0 obj1 - extend object obj0 with every field from obj1
class ExtendField : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, ExtendField, "System", "extend");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_array(arg0)) && (machine()->is_array(arg1))) {
            auto ff0 = machine()->get_array(arg0);
            auto sz0 = ff0.size();
            auto ff1 = machine()->get_array(arg1);
            auto sz1 = ff1.size();

            auto object = machine()->get_combinator("System", "object");

            // check head is an object
            if (sz0 == 0) machine()->bad(this, "invalid");
            if (ff0[0] != object) machine()->bad(this, "invalid");
            if (sz1 == 0) machine()->bad(this, "invalid");
            if (ff1[0] != object) machine()->bad(this, "invalid");
            // create field union
            unsigned int n;
            std::map<VMObjectPtr, VMObjectPtr> fields;
            for (n = 1; n < sz0; n = n + 2) {
                if ((n + 1) < sz0) fields[ff0[n]] = ff0[n + 1];
            }
            for (n = 1; n < sz1; n = n + 2) {
                if ((n + 1) < sz1) fields[ff1[n]] = ff1[n + 1];
            }
            // return object
            VMObjectPtrs oo;
            oo.push_back(object);
            for (const auto &f : fields) {
                oo.push_back(f.first);
                oo.push_back(f.second);
            }

            return machine()->create_array(oo);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::to_int x - Try and convert an object to int
class Toint : public Monadic {
public:
    MONADIC_PREAMBLE(VM_SUB_BUILTIN, Toint, "System", "to_int");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        if (machine()->is_integer(arg0)) {
            return arg0;
        } else if (machine()->is_float(arg0)) {
            auto f = machine()->get_float(arg0);
            return machine()->create_integer((int)f);
        } else if (machine()->is_char(arg0)) {
            auto c = machine()->get_char(arg0);
            auto i = convert_to_int(icu::UnicodeString() + c);
            return machine()->create_integer(i);
        } else if (machine()->is_text(arg0)) {
            auto s = machine()->get_text(arg0);
            auto i = convert_to_int(s);
            return machine()->create_integer(i);
        } else {
            throw machine()->bad_args(this, arg0);
        }
    }
};

//## System::to_float x - try and convert an object to float
class Tofloat : public Monadic {
public:
    MONADIC_PREAMBLE(VM_SUB_BUILTIN, Tofloat, "System", "to_float");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        if (machine()->is_integer(arg0)) {
            auto i = machine()->get_integer(arg0);
            return machine()->create_float(i);
        } else if (machine()->is_float(arg0)) {
            return arg0;
        } else if (machine()->is_text(arg0)) {
            auto s = machine()->get_text(arg0);
            auto i = convert_to_float(s);
            return machine()->create_float(i);
        } else {
            throw machine()->bad_args(this, arg0);
        }
    }
};

//## System::to_text x - try and convert an object to text
class Totext : public Monadic {
public:
    MONADIC_PREAMBLE(VM_SUB_BUILTIN, Totext, "System", "to_text");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        if (machine()->is_integer(arg0)) {
            auto i = machine()->get_integer(arg0);
            auto s = convert_from_int(i);
            return machine()->create_text(s);
        } else if (machine()->is_float(arg0)) {
            auto f = machine()->get_float(arg0);
            auto s = convert_from_float(f);
            return machine()->create_text(s);
        } else if (machine()->is_char(arg0)) {
            auto c = machine()->get_char(arg0);
            auto s = convert_from_char(c);
            return machine()->create_text(s);
        } else if (machine()->is_text(arg0)) {
            return arg0;
        } else {
            throw machine()->bad_args(this, arg0);
        }
    }
};

//## System::reference - an opaque reference object
class Reference : public Opaque {
public:
    OPAQUE_PREAMBLE(VM_SUB_BUILTIN, Reference, "System", "reference");

    Reference(VM *vm, const VMObjectPtr &r)
        : Opaque(VM_SUB_BUILTIN, vm, "System", "reference") {
        _ref = r;
    }

    Reference(const Reference &ref)
        : Opaque(VM_SUB_BUILTIN, ref.machine(), ref.symbol()) {
        _ref = ref.get_ref();
    }

    static VMObjectPtr create(VM *vm, const VMObjectPtr &ref) {
        return VMObjectPtr(new Reference(vm, ref));
    }

    int compare(const VMObjectPtr &o) override {
        return -1;  // XXX: fix this once
    }

    VMObjectPtr get_ref() const {
        return _ref;
    }

    void set_ref(const VMObjectPtr &r) {
        _ref = r;
    }

protected:
    VMObjectPtr _ref = nullptr;
};

//## System::ref x - create a reference object from x
class Ref : public Monadic {
public:
    MONADIC_PREAMBLE(VM_SUB_BUILTIN, Ref, "System", "ref");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        auto vm = machine();
        auto r = Reference::create(vm, arg0);
        return r;
    }
};

//## System::get_ref ref - get the stored value from ref
class Getref : public Unary {
public:
    UNARY_PREAMBLE(VM_SUB_BUILTIN, Getref, "System", "get_ref");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        symbol_t sym = machine()->enter_symbol("System", "reference");

        if ((arg0->tag() == VM_OBJECT_OPAQUE) && (arg0->symbol() == sym)) {
            auto r = std::static_pointer_cast<Reference>(arg0);
            return r->get_ref();
        } else {
            throw machine()->bad_args(this, arg0);
        }
    }
};

//## System::set_ref ref x - set reference object ref to x
class Setref : public Dyadic {
public:
    DYADIC_PREAMBLE(VM_SUB_BUILTIN, Setref, "System", "set_ref");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        symbol_t sym = machine()->enter_symbol("System", "reference");

        if ((machine()->is_opaque(arg0)) && (arg0->symbol() == sym)) {
            auto r = std::static_pointer_cast<Reference>(arg0);
            r->set_ref(arg1);
            return arg0;
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::unpack s - create a list of chars from a string
class Unpack : public Monadic {
public:
    MONADIC_PREAMBLE(VM_SUB_BUILTIN, Unpack, "System", "unpack");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        if (machine()->is_text(arg0)) {
            auto str = machine()->get_text(arg0);

            VMObjectPtrs ss;
            int len = str.length();
            for (int n = 0; n < len; n++) {
                auto c = machine()->create_char(str.char32At(n));
                ss.push_back(c);
            }
            return machine()->to_list(ss);
        } else {
            throw machine()->bad_args(this, arg0);
        }
    }
};

//## System::pack s - create a string from a list of code points
class Pack : public Monadic {
public:
    MONADIC_PREAMBLE(VM_SUB_BUILTIN, Pack, "System", "pack");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        static symbol_t _nil = 0;
        if (_nil == 0) _nil = machine()->enter_symbol("System", "nil");

        static symbol_t _cons = 0;
        if (_cons == 0) _cons = machine()->enter_symbol("System", "cons");

        icu::UnicodeString ss;
        auto a = arg0;

        while ((machine()->is_array(a))) {
            auto aa = machine()->get_array(a);
            if (aa.size() != 3) machine()->bad(this, "invalid");
            if (aa[0]->symbol() != _cons) machine()->bad(this, "invalid");
            if (aa[1]->tag() != VM_OBJECT_CHAR) machine()->bad(this, "invalid");

            auto c = machine()->get_char(aa[1]);

            ss += c;

            a = aa[2];
        }

        return VMObjectText::create(ss);
    }
};

//## System::arg n - return the n-th application argument, otherwise return
//'none'
class Arg : public Monadic {
public:
    MONADIC_PREAMBLE(VM_SUB_BUILTIN, Arg, "System", "arg");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        if (machine()->is_integer(arg0)) {
            auto i = machine()->get_integer(arg0);
            if (i < application_argc) {
                return VMObjectText::create(application_argv[i]);
            } else {
                auto none = machine()->create_none();
                return none;
            }
        } else {
            throw machine()->bad_args(this, arg0);
        }
    }
};

//## System::get_env s - return the value of environment variable s, otherwise
// return 'none'
class Getenv : public Monadic {
public:
    MONADIC_PREAMBLE(VM_SUB_BUILTIN, Getenv, "System", "get_env");

    VMObjectPtr apply(const VMObjectPtr &arg0) const override {
        if (machine()->is_text(arg0)) {
            auto t = machine()->get_text(arg0);
            char *s = unicode_to_char(t);
            char *r = std::getenv(s);
            delete s;
            if (r != nullptr) {
                return VMObjectText::create(r);  // NOTE: don't call delete on r
            } else {
                auto none = machine()->create_none();
                return none;
            }
        } else {
            throw machine()->bad_args(this, arg0);
        }
    }
};

//## System::&& - short-circuited and (deprecated)
class LazyAnd : public Binary {
public:
    BINARY_PREAMBLE(VM_SUB_BUILTIN, LazyAnd, "System", "&&");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_combinator(arg0)) &&
            (arg0->symbol() == SYMBOL_FALSE)) {
            return arg0;
        } else if ((machine()->is_combinator(arg0)) &&
                   (arg0->symbol() == SYMBOL_TRUE)) {
            VMObjectPtrs thunk;
            thunk.push_back(arg1);
            thunk.push_back(machine()->create_none());
            return machine()->create_array(thunk);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::|| - short-circuited or (deprecated)
class LazyOr : public Binary {
public:
    BINARY_PREAMBLE(VM_SUB_BUILTIN, LazyOr, "System", "||");

    VMObjectPtr apply(const VMObjectPtr &arg0,
                      const VMObjectPtr &arg1) const override {
        if ((machine()->is_combinator(arg0)) &&
            (arg0->symbol() == SYMBOL_TRUE)) {
            return arg0;
        } else if ((machine()->is_combinator(arg0)) &&
                   (arg0->symbol() == SYMBOL_FALSE)) {
            VMObjectPtrs thunk;
            thunk.push_back(arg1);
            thunk.push_back(machine()->create_none());
            return machine()->create_array(thunk);
        } else {
            throw machine()->bad_args(this, arg0, arg1);
        }
    }
};

//## System::print o0 .. on - print terms, don't escape characters or texts
class Print : public Variadic {
public:
    VARIADIC_PREAMBLE(VM_SUB_BUILTIN, Print, "System", "print");

    VMObjectPtr apply(const VMObjectPtrs &args) const override {
        icu::UnicodeString s;
        for (auto &arg : args) {
            if (machine()->is_integer(arg)) {
                s += arg->to_text();
            } else if (machine()->is_float(arg)) {
                s += arg->to_text();
            } else if (arg->tag() == VM_OBJECT_CHAR) {
                s += machine()->get_char(arg);
            } else if (machine()->is_text(arg)) {
                s += machine()->get_text(arg);
            } else {
                s += arg->to_text();
            }
        }
        std::cout << s;

        return machine()->create_none();
    }
};

//## System::get_line - read a line from standard input
class Getline : public Medadic {
public:
    MEDADIC_PREAMBLE(VM_SUB_BUILTIN, Getline, "System", "get_line");

    VMObjectPtr apply() const override {
        std::string line;
        std::getline(std::cin, line);
        icu::UnicodeString str(line.c_str());
        return machine()->create_text(str);
    }
};

//## System::format fmt x ...  - create a string from formatted string fmt and
// objects x,..
class Format : public Variadic {
public:
    VARIADIC_PREAMBLE(VM_SUB_BUILTIN, Format, "System", "format");

    VMObjectPtr apply(const VMObjectPtrs &args) const override {
        if (args.size() < 1) {
            return nullptr;
        } else {
            auto a0 = args[0];
            if (machine()->is_text(a0)) {
                auto f = machine()->get_text(a0);
                auto fmt = unicode_to_char(f);

                fmt::dynamic_format_arg_store<fmt::format_context> store;

                for (int n = 1; n < (int)args.size(); n++) {
                    auto arg = args[n];
                    if (machine()->is_integer(arg)) {
                        auto i = machine()->get_integer(arg);
                        store.push_back(i);
                    } else if (machine()->is_float(arg)) {
                        auto f = machine()->get_float(arg);
                        store.push_back(f);
                    } else if (machine()->is_char(arg)) {
                        auto c = machine()->get_char(arg);
                        auto s0 = icu::UnicodeString(c);
                        auto s1 = unicode_to_char(s0);
                        store.push_back(s1);
                        delete s1;
                    } else if (machine()->is_text(arg)) {
                        auto t = machine()->get_text(arg);
                        auto s0 = unicode_to_char(t);
                        store.push_back(s0);
                        delete s0;
                    } else {
                        auto a = arg->to_text();
                        auto s0 = unicode_to_char(a);
                        store.push_back(s0);
                        delete s0;
                    }
                }

                std::string r;
                try {
                    r = fmt::vformat(fmt, store);
                } catch (std::runtime_error &e) {
                    throw machine()->bad(this, "invalid");
                }
                auto u = icu::UnicodeString(r.c_str());
                delete fmt;

                return VMObjectText::create(u);
            } else {
                throw machine()->bad(this, "format");
            }
        }
    }
};

inline std::vector<VMObjectPtr> builtin_system(VM *vm) {
    std::vector<VMObjectPtr> oo;

    // throw combinator
    oo.push_back(VMThrow::create(vm));

    // K, Id combinators
    oo.push_back(K::create(vm));
    oo.push_back(Id::create(vm));

    // basic constants
    oo.push_back(VMObjectData::create(vm, "System", "int"));
    oo.push_back(VMObjectData::create(vm, "System", "float"));
    oo.push_back(VMObjectData::create(vm, "System", "char"));
    oo.push_back(VMObjectData::create(vm, "System", "text"));
    oo.push_back(VMObjectData::create(vm, "System", "nil"));
    oo.push_back(VMObjectData::create(vm, "System", "cons"));
    oo.push_back(VMObjectData::create(vm, "System", "none"));
    oo.push_back(VMObjectData::create(vm, "System", "true"));
    oo.push_back(VMObjectData::create(vm, "System", "false"));
    oo.push_back(VMObjectData::create(vm, "System", "tuple"));
    oo.push_back(VMObjectData::create(vm, "System", "object"));

    // operators

    oo.push_back(MinInt::create(vm));
    oo.push_back(MaxInt::create(vm));
    oo.push_back(Add::create(vm));
    oo.push_back(MonMin::create(vm));  // the dreaded operator
    oo.push_back(Min::create(vm));
    oo.push_back(Mul::create(vm));
    oo.push_back(Div::create(vm));
    oo.push_back(Mod::create(vm));

    oo.push_back(Less::create(vm));
    oo.push_back(LessEq::create(vm));
    oo.push_back(Eq::create(vm));
    oo.push_back(NegEq::create(vm));

    oo.push_back(BinAnd::create(vm));
    oo.push_back(BinOr::create(vm));
    oo.push_back(BinXOr::create(vm));
    oo.push_back(BinComplement::create(vm));
    oo.push_back(BinLeftShift::create(vm));
    oo.push_back(BinRightShift::create(vm));

    oo.push_back(Toint::create(vm));
    oo.push_back(Tofloat::create(vm));
    oo.push_back(Totext::create(vm));

    // move to string?
    oo.push_back(Unpack::create(vm));
    oo.push_back(Pack::create(vm));

    // lazy operators
    //    oo.push_back(LazyAnd::create(vm));
    //    oo.push_back(LazyOr::create(vm));

    // system info, override if sandboxed
    oo.push_back(Arg::create(vm));
    oo.push_back(Getenv::create(vm));

    // the builtin print & getline, override if sandboxed
    oo.push_back(Print::create(vm));
    oo.push_back(Getline::create(vm));
    oo.push_back(Format::create(vm));

    // references
    // oo.push_back(Reference::create(vm)); // XXXX
    oo.push_back(VMObjectStub::create(vm, "System::reference"));  // XXXX
    oo.push_back(Ref::create(vm));
    oo.push_back(Setref::create(vm));
    oo.push_back(Getref::create(vm));

    // OO fields
    oo.push_back(GetField::create(vm));
    oo.push_back(SetField::create(vm));
    oo.push_back(ExtendField::create(vm));

    return oo;
}
