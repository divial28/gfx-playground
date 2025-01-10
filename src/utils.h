#pragma once

#define EVAL_ONCE(expression)       \
    {                               \
        static bool done = false;   \
        if (!done) {                \
            done = true;            \
            (expression);           \
        }                           \
    }