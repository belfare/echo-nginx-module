#define DDEBUG 1

#include "ddebug.h"
#include "sleep.h"
#include "handler.h"

#include <nginx.h>
#include <ngx_log.h>

/* event handler for echo_sleep */

static void ngx_http_echo_post_sleep(ngx_http_request_t *r);

ngx_int_t
ngx_http_echo_exec_echo_sleep(
        ngx_http_request_t *r, ngx_http_echo_ctx_t *ctx,
        ngx_array_t *computed_args) {
    ngx_str_t                   *computed_arg;
    ngx_str_t                   *computed_arg_elts;
    float                       delay; /* in sec */

    computed_arg_elts = computed_args->elts;
    computed_arg = &computed_arg_elts[0];

    delay = atof( (char*) computed_arg->data );

    if (delay < 0.001) { /* should be bigger than 1 msec */
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                   "invalid sleep duration \"%V\"", &computed_arg_elts[0]);
        return NGX_HTTP_BAD_REQUEST;
    }

    DD("DELAY = %.02lf sec", delay);

#if defined(nginx_version) && nginx_version >= 8011

    r->main->count++;
    DD("request main count : %u", r->main->count);

#endif

    ngx_add_timer(&ctx->sleep, (ngx_msec_t) (1000 * delay));

    return NGX_DONE;
}

static void
ngx_http_echo_post_sleep(ngx_http_request_t *r) {
    ngx_http_echo_ctx_t         *ctx;

    DD("entered echo post sleep...");

    ctx = ngx_http_get_module_ctx(r, ngx_http_echo_module);
    if (ctx == NULL) {
        return ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
    }

    ctx->next_handler_cmd++;

    if (ctx->sleep.timer_set) {
        ngx_del_timer(&ctx->sleep);
    }

    ctx->sleep.timedout = 0;

    ngx_http_finalize_request(r, ngx_http_echo_handler(r));

    return;
}

void
ngx_http_echo_sleep_event_handler(ngx_event_t *ev) {
    ngx_connection_t        *c;
    ngx_http_request_t      *r;
    ngx_http_log_ctx_t      *ctx;

    r = ev->data;
    c = r->connection;
    ctx = c->log->data;
    ctx->current_request = r;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0,
            "echo sleep handler: \"%V?%V\"", &r->uri, &r->args);

    ngx_http_echo_post_sleep(r);

#if defined(nginx_version)

    ngx_http_run_posted_requests(c);

#endif

}

ngx_int_t
ngx_http_echo_exec_echo_blocking_sleep(ngx_http_request_t *r,
        ngx_http_echo_ctx_t *ctx, ngx_array_t *computed_args) {
    ngx_str_t                   *computed_arg;
    ngx_str_t                   *computed_arg_elts;
    float                       delay; /* in sec */

    computed_arg_elts = computed_args->elts;
    computed_arg = &computed_arg_elts[0];
    delay = atof( (char*) computed_arg->data );
    if (delay < 0.001) { /* should be bigger than 1 msec */
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                   "invalid sleep duration \"%V\"", &computed_arg_elts[0]);
        return NGX_HTTP_BAD_REQUEST;
    }

    DD("blocking DELAY = %.02lf sec", delay);

    ngx_msleep((ngx_msec_t) (1000 * delay));
    return NGX_OK;
}
