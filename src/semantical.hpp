#pragma once

#include "ast.hpp"
#include "environment.hpp"
#include "error.hpp"
#include "position.hpp"
#include "semantical.hpp"
#include "transform.hpp"

inline void declare(NamespacePtr env, const AstPtr &a);
inline AstPtr identify(NamespacePtr env, const AstPtr &a);

// XXX this is a concat we use pretty often and should be moved to a template?
UnicodeStrings concat(const UnicodeStrings &qq0, const UnicodeStrings &qq1) {
    UnicodeStrings qq;
    for (auto &q : qq0) {
        qq.push_back(q);
    }
    for (auto &q : qq1) {
        qq.push_back(q);
    }
    return qq;
}

enum declare_state_t {
    STATE_DECLARE_GLOBAL,
    STATE_DECLARE_FIELD,
};

class VisitDeclare : public Visit {
public:
    VisitDeclare() {
    }

    void declare(NamespacePtr &env, const AstPtr &a) {
        _spaces = env;
        set_declare_state(STATE_DECLARE_GLOBAL);
        visit(a);
    }

    // state manipulation
    void set_declare_state(declare_state_t s) {
        _declare_state = s;
    }

    declare_state_t get_declare_state() {
        return _declare_state;
    }

    void clear_qualifications() {
        _qualifications.clear();
    }

    UnicodeStrings get_qualifications() {
        return _qualifications;
    }

    void set_qualifications(UnicodeStrings qq) {
        _qualifications = qq;
    }

    // helper functions
    icu::UnicodeString qualified(const UnicodeStrings &nn,
                                 const icu::UnicodeString n) {
        icu::UnicodeString s;
        for (auto &n : nn) {
            s += n + STRING_DCOLON;
        }
        s += n;
        return s;
    }

    // visits
    void visit_expr_combinator(const Position &p, const UnicodeStrings &nn,
                               const icu::UnicodeString &n) override {
        switch (get_declare_state()) {
            case STATE_DECLARE_GLOBAL:
                try {
                    auto nn0 = concat(_qualifications, nn);
                    auto q = qualified(nn0, n);
                    ::declare(_spaces, nn0, n, q);
                } catch (ErrorSemantical &e) {
                    throw ErrorSemantical(p, "redeclaration of " + n);
                }
                break;
            case STATE_DECLARE_FIELD:
                try {
                    const UnicodeStrings oo = {"OO"};
                    auto nn0 = concat(oo, nn);
                    auto q = qualified(nn0, n);
                    ::declare_implicit(_spaces, nn0, n, q);
                } catch (ErrorSemantical &e) {
                    throw ErrorSemantical(p, "redeclaration of " + n);
                }
                break;
        }
    }

    void visit_decl_data(const Position &p, const AstPtrs &nn) override {
        if (get_declare_state() == STATE_DECLARE_GLOBAL) {
            visits(nn);
        } else {
            visit(nn[0]);
        }
    }

    void visit_decl_definition(const Position &p, const AstPtr &n,
                               const AstPtr &e) override {
        visit(n);
    }

    void visit_decl_value(const Position &p, const AstPtr &l,
                          const AstPtr &r) override {
        visit(l);
    }

    void visit_decl_operator(const Position &p, const AstPtr &c,
                             const AstPtr &e) override {
        visit(c);
    }

    void visit_decl_object(const Position &p, const AstPtr &c,
                           const AstPtrs &vv, const AstPtrs &ff,
                           const AstPtrs &ee) override {
        visit(c);
        set_declare_state(STATE_DECLARE_FIELD);
        visits(ff);
        set_declare_state(STATE_DECLARE_GLOBAL);
    }

    void visit_decl_namespace(const Position &p, const UnicodeStrings &nn,
                              const AstPtrs &dd) override {
        auto nn0 = get_qualifications();
        auto nn1 = concat(nn0, nn);
        set_qualifications(nn1);
        visits(dd);
        set_qualifications(nn0);
    }

private:
    NamespacePtr _spaces;
    UnicodeStrings _qualifications;
    declare_state_t _declare_state;
};

void declare(NamespacePtr env, const AstPtr &a) {
    VisitDeclare declare;
    declare.declare(env, a);
}

enum identify_state_t {
    STATE_IDENTIFY_USE,
    STATE_IDENTIFY_PATTERN,
    STATE_IDENTIFY_FIELD,
};

/*
 * Stuff this pass does:
 * + Check that all symbols have bindings.
 * + Fully qualify declarations.
 * + Flatten the namespace.
 */
class RewriteIdentify : public Rewrite {
public:
    AstPtr identify(NamespacePtr env, AstPtr a) {
        _identify_state = STATE_IDENTIFY_USE;
        _range = range_nil(env);
        _counter = 0;
        return rewrite(a);
    }

    // variable
    icu::UnicodeString fresh_variable() {
        auto v = icu::UnicodeString("V_") + convert_from_int(_counter);
        _counter++;
        return v;
    }

    // state
    void set_identify_state(identify_state_t s) {
        _identify_state = s;
    }

    identify_state_t get_identify_state() {
        return _identify_state;
    }

    // range manipulation
    void declare(const Position &p, const icu::UnicodeString &k,
                 const icu::UnicodeString &v) {
        try {
            ::declare(_range, k, v);
        } catch (ErrorSemantical &e) {
            throw ErrorSemantical(p, "redeclaration of " + k);
        }
    }

    bool has(const icu::UnicodeString &k) {
        icu::UnicodeString tmp = ::get(_range, k);
        return (tmp != "");
    }

    icu::UnicodeString get(const Position &p, const icu::UnicodeString &k) {
        icu::UnicodeString tmp = ::get(_range, k);
        if (tmp == "") {
            throw ErrorSemantical(p, "undeclared " + k);
        } else {
            return tmp;
        }
    }

    icu::UnicodeString get(const Position &p, const UnicodeStrings &kk,
                           const icu::UnicodeString &k) {
        icu::UnicodeString tmp = ::get(_range, kk, k);
        if (tmp == "") {
            throw ErrorSemantical(p, "undeclared " + k);
        } else {
            return tmp;
        }
    }

    void enter_range() {
        _range = ::enter_range(_range);
    }

    void leave_range() {
        _range = ::leave_range(_range);
    }

    void add_using(const UnicodeStrings &nn) {
        ::add_using(_range, nn);
    }

    // namespaces
    UnicodeStrings get_namespace() {
        return _namespace;
    }

    void set_namespace(UnicodeStrings n) {
        _namespace = n;
    }

    // push/pop declarations
    void push_declaration(const AstPtr &d) {
        _declarations.push_back(d);
    }

    AstPtrs pop_declarations() {
        return _declarations;
    }

    // rewrites
    AstPtr rewrite_expr_variable(const Position &p,
                                 const icu::UnicodeString &v) override {
        switch (get_identify_state()) {
            case STATE_IDENTIFY_USE: {
                auto v1 = get(p, v);
                return AstExprVariable::create(p, v1);
            } break;
            case STATE_IDENTIFY_PATTERN: {
                auto fv = fresh_variable();
                declare(p, v, fv);
                return AstExprVariable::create(p, fv);
            } break;
            default:
                PANIC("unknown state");
                return nullptr;
                break;
        }
    }

    AstPtr rewrite_expr_combinator(const Position &p, const UnicodeStrings &nn,
                                   const icu::UnicodeString &t) override {
        UnicodeStrings ee;
        switch (get_identify_state()) {
            case STATE_IDENTIFY_PATTERN:
            case STATE_IDENTIFY_USE: {
                auto v = get(p, nn, t);
                auto c = AstExprCombinator::create(p, ee, v);
                return c;
            } break;
            default:
                PANIC("unknown state");
                return nullptr;
                break;
        }
        return nullptr;  // play nice with -Wall
    }

    AstPtr rewrite_expr_operator(const Position &p, const UnicodeStrings &nn,
                                 const icu::UnicodeString &t) override {
        UnicodeStrings ee;
        switch (get_identify_state()) {
            case STATE_IDENTIFY_PATTERN:
            case STATE_IDENTIFY_USE: {
                auto v = get(p, nn, t);
                auto c = AstExprOperator::create(p, ee, v);
                return c;
            } break;
            default:
                PANIC("unknown state");
                return nullptr;
                break;
        }
        return nullptr;  // play nice with -Wall
    }

    AstPtr rewrite_expr_match(const Position &p, const AstPtrs &mm,
                              const AstPtr &g, const AstPtr &e) override {
        enter_range();
        set_identify_state(STATE_IDENTIFY_PATTERN);
        auto mm0 = rewrites(mm);
        set_identify_state(STATE_IDENTIFY_USE);
        auto g0 = rewrite(g);
        set_identify_state(STATE_IDENTIFY_USE);
        auto e0 = rewrite(e);
        leave_range();
        return AstExprMatch::create(p, mm0, g0, e0);
    }

    AstPtr rewrite_expr_let(const Position &p, const AstPtrs &lhs,
                            const AstPtr &rhs, const AstPtr &body) override {
        set_identify_state(STATE_IDENTIFY_USE);
        auto rhs0 = rewrite(rhs);
        enter_range();
        set_identify_state(STATE_IDENTIFY_PATTERN);
        auto lhs0 = rewrites(lhs);
        set_identify_state(STATE_IDENTIFY_USE);
        auto body0 = rewrite(body);
        leave_range();
        return AstExprLet::create(p, lhs0, rhs0, body0);
    }

    AstPtr rewrite_expr_tag(const Position &p, const AstPtr &e,
                            const AstPtr &t) override {
        set_identify_state(STATE_IDENTIFY_PATTERN);
        auto e0 = rewrite(e);
        set_identify_state(STATE_IDENTIFY_USE);
        auto t0 = rewrite(t);
        set_identify_state(STATE_IDENTIFY_PATTERN);
        return AstExprTag::create(p, e0, t0);
    }

    AstPtr rewrite_directive_using(const Position &p,
                                   const UnicodeStrings &nn) override {
        add_using(nn);
        return AstDirectUsing::create(p, nn);
    }

    AstPtr rewrite_decl_data(const Position &p, const AstPtrs &ee) override {
        if (get_identify_state() == STATE_IDENTIFY_FIELD) {
            set_identify_state(STATE_IDENTIFY_USE);
            auto ee0 = rewrites(ee);
            auto a = AstDeclData::create(p, ee0);
            set_identify_state(STATE_IDENTIFY_FIELD);
            return a;
        } else {
            set_identify_state(STATE_IDENTIFY_USE);
            auto ee0 = rewrites(ee);
            auto a = AstDeclData::create(p, ee0);
            push_declaration(a);
            return a;
        }
    }

    AstPtr rewrite_decl_definition(const Position &p, const AstPtr &n,
                                   const AstPtr &e) override {
        if (get_identify_state() == STATE_IDENTIFY_FIELD) {
            set_identify_state(STATE_IDENTIFY_USE);
            auto n0 = rewrite(n);
            auto e0 = rewrite(e);
            auto a = AstDeclDefinition::create(p, n0, e0);
            set_identify_state(STATE_IDENTIFY_FIELD);
            return a;
        } else {
            auto n0 = rewrite(n);
            auto e0 = rewrite(e);
            auto a = AstDeclDefinition::create(p, n0, e0);
            push_declaration(a);
            set_identify_state(STATE_IDENTIFY_USE);
            return a;
        }
    }

    AstPtr rewrite_decl_value(const Position &p, const AstPtr &l,
                              const AstPtr &r) override {
        set_identify_state(STATE_IDENTIFY_USE);
        auto l0 = rewrite(l);
        auto r0 = rewrite(r);
        auto a = AstDeclValue::create(p, l0, r0);
        push_declaration(a);
        return a;
    }

    AstPtr rewrite_decl_operator(const Position &p, const AstPtr &c,
                                 const AstPtr &e) override {
        set_identify_state(STATE_IDENTIFY_USE);
        auto c0 = rewrite(c);
        auto e0 = rewrite(e);
        auto a = AstDeclOperator::create(p, c0, e0);
        push_declaration(a);
        return a;
    }

    AstPtr rewrite_decl_object(const Position &p, const AstPtr &c,
                               const AstPtrs &vv, const AstPtrs &ff,
                               const AstPtrs &ee) override {
        set_identify_state(STATE_IDENTIFY_USE);
        auto c0 = rewrite(c);
        enter_range();
        set_identify_state(STATE_IDENTIFY_PATTERN);
        auto vv0 = rewrites(vv);
        set_identify_state(STATE_IDENTIFY_USE);
        auto ee0 = rewrites(ee);
        set_identify_state(STATE_IDENTIFY_FIELD);
        auto ff0 = rewrites(ff);
        leave_range();
        auto a = AstDeclObject::create(p, c0, vv0, ff0, ee0);
        push_declaration(a);
        set_identify_state(STATE_IDENTIFY_USE);
        return a;
    }

    AstPtr rewrite_decl_namespace(const Position &p, const UnicodeStrings &nn,
                                  const AstPtrs &dd) override {
        auto nn0 = get_namespace();
        auto nn1 = concat(nn0, nn);
        set_namespace(nn1);
        enter_range();
        add_using(nn1);
        auto dd0 = rewrites(dd);
        leave_range();
        set_namespace(nn0);
        return AstDeclNamespace::create(p, nn, dd0);
    }

    AstPtr rewrite_wrapper(const Position &p, const AstPtrs &dd) override {
        auto dd0 = rewrites(dd);
        auto aa = pop_declarations();
        return AstWrapper::create(p, aa);
    }

private:
    identify_state_t _identify_state;
    RangePtr _range;
    AstPtrs _declarations;
    UnicodeStrings _namespace;
    int _counter;
};

AstPtr identify(NamespacePtr env, const AstPtr &a) {
    RewriteIdentify identify;
    return identify.identify(env, a);
}
