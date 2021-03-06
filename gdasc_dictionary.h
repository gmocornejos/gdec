#ifndef DICTIONARY_H
#define DICTIONARY_H

#define DICT_EMPTY       5000
#define NO_KEY           5001
#define DICT_INDEX_ERROR 5003

#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define DICT_GROW_RATIO 1.2

#define DICTIONARY_DECLARE(key_t, value_t, name)                    \
typedef struct{                                                     \
    key_t key;                                                      \
    value_t value;                                                  \
} name##_entry;                                                     \
typedef struct name name;                                           \
typedef name##_entry * name##_itr;                                  \
struct name{                                                        \
    pthread_mutex_t mutex;                                          \
    size_t entry_size;                                              \
    size_t length;                                                  \
    size_t capacity;                                                \
    name##_entry * begin;                                           \
    name##_entry * end;                                             \
    int (*cmp) (key_t, key_t);                                      \
    name *         (*constructor) (int (*cmp)(key_t, key_t));       \
    name##_entry * (*set)         (name *, key_t, value_t);         \
    value_t        (*get)         (name *, key_t);                  \
    value_t        (*pop)         (name *, key_t);                  \
    int            (*index)       (name *, key_t);                  \
    name##_entry   (*popindex)    (name *, int);                    \
    name##_entry * (*clear)       (name *);                         \
    name *         (*copy)        (name *);                         \
    name##_entry * (*update)      (name *, name *);                 \
    void           (*protect)     (name *);                         \
    void           (*release)     (name *);                         \
    void           (*destroy)     (name *);                         \
};                                                                  \
extern name name##_class;                                           \


#define DICTIONARY_GEN_CODE(key_t, value_t, name)                   \
name##_entry * name##_set(name * self , key_t k, value_t v){    \
    for(name##_itr i = self->begin; i != self->end; ++i){           \
        if(self->cmp(i->key, k) == 0){                              \
            i->value = v;                                           \
            return self->begin;                                     \
        }                                                           \
    }                                                               \
    self->end -> key   = k;                                            \
    self->end -> value = v;                                            \
    self->end++;                                                    \
    if(++self->length == self->capacity){                           \
        ++self->capacity;                                           \
        self->capacity *= DICT_GROW_RATIO;                               \
        self->begin = realloc(self->begin, self->entry_size * self->capacity);\
        self->end = self->begin + self->length;                     \
    }                                                               \
    return self->begin;                                             \
}                                                                   \
\
value_t  name##_get(name * self, key_t k){                      \
    if(self->length == 0){                                          \
        fprintf(stderr, "Error in get method: dictionary empty");   \
        exit(DICT_EMPTY);                                           \
    }                                                               \
    for(name##_itr i = self->begin; i != self->end; ++i){           \
        if(self->cmp(i->key, k) == 0){                              \
            return i->value;                                        \
        }                                                           \
    }                                                               \
    fprintf(stderr, "Error in get method: requested non-existent key\n");\
    exit(NO_KEY);                                                   \
}                                                                   \
\
value_t name##_pop(name * self, key_t k){                           \
    if(self->length == 0){                                          \
        fprintf(stderr, "Error in pop method: dictionary empty");   \
        exit(DICT_EMPTY);                                           \
    }                                                               \
    for(int i = 0; i != self->length; ++i){                         \
        if(self->cmp(self->begin[i].key, k) == 0){                  \
            value_t v = self->begin[i].value;                       \
            --self->length;                                         \
            memmove(self->begin + i, self->begin + i+1, self->entry_size * (self->length - i));\
            if(self->capacity > DICT_GROW_RATIO * self->length){         \
                self->capacity = self->length + 1;                  \
                self->begin = realloc(self->begin, self->entry_size * self->capacity);\
            }                                                       \
            self->end = self->begin + self->length;                 \
            return v;                                               \
        }                                                           \
    }                                                               \
    fprintf(stderr, "Error in pop method: requested non-existent key\n");\
    exit(NO_KEY);                                                   \
}                                                                   \
\
int name##_index(name * self, key_t k){                             \
    int r_value = -1;                                               \
    for(int i = 0; i != self->length; ++i){                         \
        if(self->cmp(self->begin[i].key, k) == 0){                  \
            r_value = i;                                            \
            break;                                                  \
        }                                                           \
    }                                                               \
    return r_value;                                                 \
}                                                                   \
\
name##_entry name##_popindex(name * self, int index){               \
    if(self->length == 0){                                          \
        fprintf(stderr, "Error in popindex method: dictionary empty");\
        exit(DICT_EMPTY);                                           \
    }                                                               \
    index = index >= 0 ? index : self->length + index;              \
    if(index >= self->length || index < 0){                         \
        fprintf(stderr, "Error in popindex method: index bigger or equal than length\n");\
        exit(DICT_INDEX_ERROR);                                     \
    }                                                               \
    name##_entry e = self->begin[index];                            \
    --self->length;                                                 \
    memmove(self->begin + index, self->begin + index+1, self->entry_size * (self->length - index));\
    if(self->capacity > DICT_GROW_RATIO * self->length){                 \
        self->capacity = self->length + 1;                          \
        self->begin = realloc(self->begin, self->entry_size * self->capacity); \
    }                                                               \
    self->end = self->begin + self->length;                         \
    return e;                                                       \
}                                                                   \
\
name##_entry * name##_clear(name * self){                           \
    self->length = 0;                                               \
    self->capacity = 1;                                             \
    self->begin = realloc(self->begin, self->entry_size * self->capacity);\
    self->end = self->begin;                                        \
    return self->begin;                                             \
}                                                                   \
\
name * name##_copy(name * self){                                    \
    name * copy = malloc(sizeof(name));                             \
    *copy = *self;                                                  \
    pthread_mutex_init(&(copy->mutex), NULL);                       \
    copy->begin = malloc(self->entry_size * self->capacity);        \
    copy->end = copy->begin + copy->length;                         \
    memcpy(copy->begin, self->begin, self->length * self->entry_size);\
    return copy;                                                    \
}                                                                   \
\
name##_entry * name##_update(name * self, name * other){            \
    name##_itr s;                                                   \
    for(name##_itr o = other->begin; o != other->end; ++o){         \
        for(s = self->begin; s != self->end; ++s)                   \
            if(self->cmp(o->key, s->key) == 0){                     \
                s->value = o->value;                                \
                break;                                              \
            }                                                       \
        if(s == self->end){                                         \
            self->end -> key   = o->key;                            \
            self->end -> value = o->value;                          \
            self->end++;                                            \
            if(++self->length == self->capacity){                   \
                ++self->capacity;                                   \
                self->capacity *= DICT_GROW_RATIO;                       \
                self->begin = realloc(self->begin, self->entry_size * self->capacity);\
                self->end = self->begin + self->length;             \
            }                                                       \
        }                                                           \
    }                                                               \
    return self->begin;                                             \
}\
\
void name##_protect(name * self){                                   \
    pthread_mutex_lock(&(self->mutex));                             \
}                                                                   \
\
void name##_release(name * self){                                   \
    pthread_mutex_unlock(&(self->mutex));                           \
}                                                                   \
void name##_destroy(name * self){                                   \
    pthread_mutex_lock(&(self->mutex));                             \
    free(self->begin);                                              \
    pthread_mutex_destroy(&(self->mutex));                          \
    free(self);                                                     \
}                                                                   \
\
name * name##_constructor(int (*cmp)(key_t, key_t)){                \
    name * self = malloc(sizeof(name));                             \
    *self = name##_class;                                           \
    self -> cmp = cmp;                                              \
    self -> begin    = malloc(self->entry_size * self->capacity);   \
    self -> end      = self -> begin;                               \
    return self;                                                    \
}                                                                   \
\
name name##_class = { PTHREAD_MUTEX_INITIALIZER,                    \
                      sizeof(name##_entry),                         \
                      0,                                            \
                      1,                                            \
                      NULL,                                         \
                      NULL,                                         \
                      NULL,                                         \
                      name##_constructor,                           \
                      name##_set,                                   \
                      name##_get,                                   \
                      name##_pop,                                   \
                      name##_index,                                 \
                      name##_popindex,                              \
                      name##_clear,                                 \
                      name##_copy,                                  \
                      name##_update,                                \
                      name##_protect,                               \
                      name##_release,                               \
                      name##_destroy                                \
                    };
#endif
