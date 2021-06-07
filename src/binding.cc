#include <cmath>
#include <node_api.h>
#include "minisat/core/Solver.h"

#define NAPI_CALL(env, call)                                      \
  do {                                                            \
    napi_status status = (call);                                  \
    if (status != napi_ok) {                                      \
      const napi_extended_error_info* error_info = NULL;          \
      napi_get_last_error_info((env), &error_info);               \
      bool is_pending;                                            \
      napi_is_exception_pending((env), &is_pending);              \
      if (!is_pending) {                                          \
        const char* message = (error_info->error_message == NULL) \
            ? "empty error message"                               \
            : error_info->error_message;                          \
        napi_throw_error((env), NULL, message);                   \
        return NULL;                                              \
      }                                                           \
    }                                                             \
  } while(0)

static void
Solver_delete(napi_env, void* finalize_data, void* finalize_hint) {
  Minisat::Solver* solver = reinterpret_cast<Minisat::Solver*>(finalize_data);
  delete solver;
}

static napi_value
Solver_new(napi_env env, napi_callback_info info) {
  Minisat::Solver* solver = new Minisat::Solver();
  napi_value self;
  NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &self, nullptr));
  NAPI_CALL(env, napi_wrap(env, self, solver, Solver_delete, nullptr, nullptr));
  return self;
}

static napi_value
Solver_newVar(napi_env env, napi_callback_info info) {
  napi_value self;

  NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &self, nullptr));

  Minisat::Solver* solver = nullptr;
  NAPI_CALL(env, napi_unwrap(env, self, reinterpret_cast<void**>(&solver)));

  Minisat::Var var = solver->newVar();

  napi_value result;
  NAPI_CALL(env, napi_create_int32(env, var, &result));
  return result;
}

static napi_value
Solver_addClause(napi_env env, napi_callback_info info) {
  napi_value self;
  size_t argc = 1;
  napi_value argv[1];

  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &self, nullptr));
  Minisat::Solver* solver = nullptr;
  NAPI_CALL(env, napi_unwrap(env, self, reinterpret_cast<void**>(&solver)));

  uint32_t len;
  NAPI_CALL(env, napi_get_array_length(env, argv[0], &len));
  Minisat::vec<Minisat::Lit> lits;
  for (uint32_t i = 0; i < len; i++) {
    napi_value elem;
    NAPI_CALL(env, napi_get_element(env, argv[0], i, &elem));
    int32_t var;
    NAPI_CALL(env, napi_get_value_int32(env, elem, &var));
    double sign;
    NAPI_CALL(env, napi_get_value_double(env, elem, &sign));
    // detect -0.0. asking for trouble?
    lits.push(!std::signbit(sign) ? Minisat::mkLit(var) : ~Minisat::mkLit(-var));
  }
  solver->addClause(lits);

  return nullptr;
}

static napi_value
Solver_simplify(napi_env env, napi_callback_info info) {
  napi_value self;

  NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &self, nullptr));

  Minisat::Solver* solver = nullptr;
  NAPI_CALL(env, napi_unwrap(env, self, reinterpret_cast<void**>(&solver)));

  bool s = solver->simplify();

  napi_value result;
  NAPI_CALL(env, napi_get_boolean(env, s, &result));
  
  return result;
}

static napi_value
Solver_solve(napi_env env, napi_callback_info info) {
  napi_value self;

  NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &self, nullptr));

  Minisat::Solver* solver = nullptr;
  NAPI_CALL(env, napi_unwrap(env, self, reinterpret_cast<void**>(&solver)));

  bool s = solver->solve();

  napi_value result;
  NAPI_CALL(env, napi_get_boolean(env, s, &result));
  
  return result;
}

static napi_value
Solver_okay(napi_env env, napi_callback_info info) {
  napi_value self;

  NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &self, nullptr));

  Minisat::Solver* solver = nullptr;
  NAPI_CALL(env, napi_unwrap(env, self, reinterpret_cast<void**>(&solver)));

  bool s = solver->okay();

  napi_value result;
  NAPI_CALL(env, napi_get_boolean(env, s, &result));
  
  return result;
}

napi_value create_addon(napi_env env) {
  napi_value result;
  NAPI_CALL(env, napi_create_object(env, &result));

  napi_property_descriptor properties[] = {
    { "solve", .method = Solver_solve },
    { "simplify", .method = Solver_simplify },
    { "okay", .method = Solver_okay },
    { "newVar", .method = Solver_newVar },
    { "addClause", .method = Solver_addClause },
  };

  napi_value class_Solver;
  NAPI_CALL(env, napi_define_class(
        env,
        "Solver",
        NAPI_AUTO_LENGTH,
        Solver_new,
        nullptr /* data */,
        sizeof(properties) / sizeof(*properties),
        properties,
        &class_Solver));

  NAPI_CALL(env, napi_set_named_property(env,
                                         result,
                                         "Solver",
                                         class_Solver));

  return result;
}

NAPI_MODULE_INIT() {
  // This function body is expected to return a `napi_value`.
  // The variables `napi_env env` and `napi_value exports` may be used within
  // the body, as they are provided by the definition of `NAPI_MODULE_INIT()`.
  return create_addon(env);
}
