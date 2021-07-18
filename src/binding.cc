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
    lits.push(var > 0 ? Minisat::mkLit(var - 1) : ~Minisat::mkLit(-var - 1));
  }
  solver->addClause_(lits);

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

struct Solver_solve__async_data {
  napi_async_work request;
  Minisat::Solver* solver;
  napi_deferred deferred;
  napi_ref ref;
  Minisat::lbool result;
};

void Solver_solve__async(napi_env env, void* data) {
  Solver_solve__async_data* async_data = reinterpret_cast<Solver_solve__async_data*>(data);
  Minisat::vec<Minisat::Lit> dummy;
  async_data->result = async_data->solver->solveLimited(dummy);
}
void Solver_solve__complete(napi_env env, napi_status status, void* data) {
  Solver_solve__async_data* async_data = reinterpret_cast<Solver_solve__async_data*>(data);
  Minisat::Solver* solver = async_data->solver;
  napi_deferred deferred = async_data->deferred;
  Minisat::lbool result = async_data->result;
  napi_delete_async_work(env, async_data->request);
  napi_delete_reference(env, async_data->ref);
  delete async_data;
  if (status != napi_ok) {
    napi_throw_type_error(env, NULL, "Solve callback failed.");
    return;
  }

  napi_value result_value;
  if (result == Minisat::l_Undef)
    napi_get_undefined(env, &result_value);
  else
    napi_get_boolean(env, result == Minisat::l_True, &result_value);
  napi_resolve_deferred(env, deferred, result_value);
}


static napi_value
Solver_solve(napi_env env, napi_callback_info info) {
  napi_value self;

  NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &self, nullptr));

  Minisat::Solver* solver = nullptr;
  NAPI_CALL(env, napi_unwrap(env, self, reinterpret_cast<void**>(&solver)));
  solver->clearInterrupt();

  napi_value resource_name;
  NAPI_CALL(env, napi_create_string_utf8(env, "Minisat::Solver::solve", NAPI_AUTO_LENGTH, &resource_name));

  Solver_solve__async_data* async_data = new Solver_solve__async_data;
  async_data->solver = solver;
  napi_value promise;
  NAPI_CALL(env, napi_create_promise(env, &async_data->deferred, &promise));

  NAPI_CALL(env, napi_create_reference(env, self, 1, &async_data->ref));
  NAPI_CALL(env, napi_create_async_work(env, nullptr, resource_name, Solver_solve__async, Solver_solve__complete, async_data, &async_data->request));


  NAPI_CALL(env, napi_queue_async_work(env, async_data->request));

  return promise;
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

static napi_value
Solver_value(napi_env env, napi_callback_info info) {
  napi_value self;

  size_t argc = 1;
  napi_value argv[1];

  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &self, nullptr));

  Minisat::Solver* solver = nullptr;
  NAPI_CALL(env, napi_unwrap(env, self, reinterpret_cast<void**>(&solver)));

  int32_t var;
  NAPI_CALL(env, napi_get_value_int32(env, argv[0], &var));

  Minisat::lbool val = solver->modelValue(var > 0 ? Minisat::mkLit(var - 1) : ~Minisat::mkLit(-var - 1));

  napi_value result;
  if (val == Minisat::l_Undef)
    NAPI_CALL(env, napi_get_undefined(env, &result));
  else
    NAPI_CALL(env, napi_get_boolean(env, val == Minisat::l_True, &result));
  return result;
}

static napi_value
Solver_interrupt(napi_env env, napi_callback_info info) {
  napi_value self;

  NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &self, nullptr));

  Minisat::Solver* solver = nullptr;
  NAPI_CALL(env, napi_unwrap(env, self, reinterpret_cast<void**>(&solver)));
  solver->interrupt();

  return nullptr;
}

static napi_value
Solver_printStats(napi_env env, napi_callback_info info) {
  napi_value self;

  NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &self, nullptr));

  Minisat::Solver* solver = nullptr;
  NAPI_CALL(env, napi_unwrap(env, self, reinterpret_cast<void**>(&solver)));
  solver->printStats();

  return nullptr;
}

static napi_value
Solver_stats(napi_env env, napi_callback_info info) {
  napi_value self;

  NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &self, nullptr));

  Minisat::Solver* solver = nullptr;
  NAPI_CALL(env, napi_unwrap(env, self, reinterpret_cast<void**>(&solver)));

  napi_value ret;
  NAPI_CALL(env, napi_create_object(env, &ret));

  napi_value num;
  NAPI_CALL(env, napi_create_int32(env, solver->starts, &num));
  NAPI_CALL(env, napi_set_named_property(env, ret, "restarts", num));

  NAPI_CALL(env, napi_create_int32(env, solver->conflicts, &num));
  NAPI_CALL(env, napi_set_named_property(env, ret, "conflicts", num));

  NAPI_CALL(env, napi_create_int32(env, solver->decisions, &num));
  NAPI_CALL(env, napi_set_named_property(env, ret, "decisions", num));

  NAPI_CALL(env, napi_create_int32(env, solver->propagations, &num));
  NAPI_CALL(env, napi_set_named_property(env, ret, "propagations", num));

  return ret;
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
    { "value", .method = Solver_value },
    { "interrupt", .method = Solver_interrupt },
    { "printStats", .method = Solver_printStats },
    { "stats", .getter = Solver_stats },
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
