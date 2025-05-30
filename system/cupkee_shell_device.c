/* GPLv2 License
 *
 * Copyright (C) 2016-2018 Lixing Ding <ding.lixing@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 **/

#include <cupkee.h>

#include "cupkee_shell_util.h"
#include "cupkee_shell_device.h"

static int device_setup(void *entry, env_t *env, val_t *setting)
{
    object_iter_t it;
    const char *key;
    val_t *val;

    (void) env;

    if (object_iter_init(&it, setting)) {
        return -CUPKEE_EINVAL;
    }

    while (object_iter_next(&it, &key, &val)) {
        if (val_is_number(val)) {
            cupkee_prop_set(entry, key, CUPKEE_OBJECT_ELEM_INT, val_2_integer(val));
        } else
        if (val_is_array(val)){
            array_t *array = (array_t *)val_2_intptr(val);
            val_t   *elem;
            int i = 0;

            for (elem = array_get(array, i); NULL != (elem = array_get(array, i)); i++) {
                if (val_is_number(elem)) {
                    if (cupkee_prop_set(entry, key, CUPKEE_OBJECT_ELEM_INT, val_2_integer(elem)) < 1) {
                        break;
                    }
                }
            }
        } else {
            const char *s = val_2_cstring(val);
            if (s) {
                cupkee_prop_set(entry, key, CUPKEE_OBJECT_ELEM_STR, (intptr_t)s);
            }
        }
    }

    return CUPKEE_OK;
}

static val_t native_device_enable(env_t *env, int ac, val_t *av)
{
    void *dev;

    (void) env;

    if (NULL == (dev= cupkee_shell_object_entry(&ac, &av))) {
        return VAL_FALSE;
    }

    if (ac > 0 && val_is_object(av)) {
        if (cupkee_device_is_enabled(dev)) {
            return VAL_FALSE;
        }
        if (0 != device_setup(dev, env, av)) {
            return VAL_FALSE;
        }
    }

    return cupkee_device_enable(dev) == 0 ? VAL_TRUE : VAL_FALSE;
}

static val_t native_device_disable(env_t *env, int ac, val_t *av)
{
    void *dev;

    (void) env;

    if (NULL == (dev= cupkee_shell_object_entry(&ac, &av))) {
        return VAL_FALSE;
    }

    return cupkee_device_disable(dev) == 0 ? VAL_TRUE : VAL_FALSE;
}

static int device_prop_get(void *entry, const char *key, val_t *prop)
{
    (void) entry;

    if (!strcmp(key, "enable")) {
        val_set_native(prop, (intptr_t)native_device_enable);
        return 1;
    } else
    if (!strcmp(key, "disable")) {
        val_set_native(prop, (intptr_t)native_device_disable);
        return 1;
    } else {
        return 0;
    }
}

static const cupkee_meta_t device_meta = {
    .prop_get = device_prop_get
};

void cupkee_shell_init_device(void)
{
    cupkee_object_set_meta(cupkee_device_tag(), (void *)&device_meta);
}

val_t native_create_device(env_t *env, int ac, val_t *av)
{
    const char *type;
    int inst;
    void *dev;

    if (ac > 0 && val_is_string(av)) {
        type = val_2_cstring(av);
        ac--; av++;
    } else {
        return VAL_UNDEFINED;
    }

    if (ac > 0 && val_is_number(av)) {
        inst = val_2_integer(av);
    } else {
        inst = 0;
    }

    dev = cupkee_device_request(type, inst);
    if (!dev) {
        return VAL_UNDEFINED;
    }

    return cupkee_shell_object_create(env, dev);
}


#if 0
typedef union device_handle_set_t {
    intptr_t param;
    uint8_t  handles[DEVICE_EVENT_MAX];
} device_handle_set_t;

static int device_is_true(intptr_t ptr);
static void device_op_prop(void *env, intptr_t id, val_t *name, val_t *prop);
static void device_op_elem(void *env, intptr_t id, val_t *which, val_t *elem);
static val_t *device_op_elem_ref(void *env, intptr_t id, val_t *key);
static void device_elem_op_set(void *env, intptr_t id, val_t *val, val_t *res);

static const char *device_event_names[] = {
    "error", "data", "drain", "ready"
};

static const val_foreign_op_t device_op = {
    .is_true = device_is_true,
    .prop = device_op_prop,
    .elem = device_op_elem,
    .elem_ref = device_op_elem_ref,
};

static const val_foreign_op_t device_elem_op = {
    .set = device_elem_op_set
};

static void device_list(void)
{
    const cupkee_device_desc_t *desc;
    int i = 0;

    console_log_sync("\r\n%8s%6s%6s%6s:%s\r\n", "DEVICE", "CONF", "INST");
    while ((desc = cupkee_device_query_by_index(i++)) != NULL) {
        console_log_sync("%8s%6d%6d%6d:%s\r\n", desc->name, desc->conf_num, desc->inst_max);
    }
}

static int device_is_true(intptr_t ptr)
{
    (void) ptr;

    return 0;
}

static void device_elem_op_set(void *env, intptr_t id, val_t *val, val_t *res)
{
    cupkee_device_t *dev;
    int index = cupkee_device_elem_index(id, &dev);

    (void) env;

    if (dev && val_is_number(val)) {
        if (0 < cupkee_device_set(dev, index, val_2_integer(val))) {
            *res = *val;
            return;
        }
    }
    *res = VAL_UNDEFINED;
}

static void device_op_prop(void *env, intptr_t id, val_t *name, val_t *prop)
{
    cupkee_device_t *dev = cupkee_device_block(id);
    const char *prop_name = val_2_cstring(name);

    (void) env;

    if (dev && prop_name) {
        if (!strcmp(prop_name, "query")) {
            val_set_native(prop, (intptr_t)native_device_query);
            return;
        } else
        if (!strcmp(prop_name, "read")) {
            val_set_native(prop, (intptr_t)native_device_read);
            return;
        } else
        if (!strcmp(prop_name, "write")) {
            val_set_native(prop, (intptr_t)native_device_write);
            return;
        } else
        if (!strcmp(prop_name, "get")) {
            val_set_native(prop, (intptr_t)native_device_get);
            return;
        } else
        if (!strcmp(prop_name, "set")) {
            val_set_native(prop, (intptr_t)native_device_set);
            return;
        } else
        if (!strcmp(prop_name, "config")) {
            val_set_native(prop, (intptr_t)native_device_config);
            return;
        } else
        if (!strcmp(prop_name, "enable")) {
            val_set_native(prop, (intptr_t)native_device_enable);
            return;
        } else
        if (!strcmp(prop_name, "disable")) {
            val_set_native(prop, (intptr_t)native_device_disable);
            return;
        } else
        if (!strcmp(prop_name, "listen")) {
            val_set_native(prop, (intptr_t)native_device_listen);
            return;
        } else
        if (!strcmp(prop_name, "ignore")) {
            val_set_native(prop, (intptr_t)native_device_ignore);
            return;
        } else
        if (!strcmp(prop_name, "isEnabled")) {
            val_set_native(prop, (intptr_t)native_device_is_enabled);
            return;
        } else
        if (!strcmp(prop_name, "destroy")) {
            val_set_native(prop, (intptr_t)native_device_destroy);
            return;
        } else
        if (!strcmp(prop_name, "error")) {
            val_set_number(prop, dev->error);
            return;
        } else
        if (!strcmp(prop_name, "instance")) {
            val_set_number(prop, dev->instance);
            return;
        } else
        if (!strcmp(prop_name, "type")) {
            val_set_foreign_string(prop, (intptr_t) dev->desc->name);
            return;
        }
    }
    val_set_undefined(prop);
}

static void device_op_elem(void *env, intptr_t id, val_t *which, val_t *elem)
{
    if (val_is_number(which)) {
        cupkee_device_t *dev = cupkee_device_block(id);
        uint32_t val;

        if (dev && 0 < cupkee_device_get(dev, val_2_integer(which), &val)) {
            val_set_number(elem, val);
        } else {
            val_set_undefined(elem);
        }
    } else {
        device_op_prop(env, id, which, elem);
    }
}

static val_t *device_op_elem_ref(void *env, intptr_t id, val_t *key)
{
    cupkee_device_t *dev = cupkee_device_block(id);
    int index;

    if (dev && val_is_number(key)) {
        index = val_2_integer(key);
    } else {
        return NULL;
    }

    *key = val_create(env, &device_elem_op, cupkee_device_elem_id(dev, index));

    return key;
}

static void device_get_all(cupkee_device_t *dev, env_t *env, val_t *result)
{
    int status;
    uint32_t data;

    status = cupkee_device_get(dev, -1, &data);
    if (status > 0) {
        // support combine read
        val_set_number(result, data);
        return;
    } else
    if (status == 0 && dev->driver->size){
        int i, size;
        array_t *list;

        size = dev->driver->size(dev->instance);
        list = _array_create(env, size);
        if (list) {
            for (i = 0; i < size; i++) {
                if (cupkee_device_get(dev, i, &data) > 0) {
                    val_set_number(_array_elem(list, i), data);
                } else {
                    val_set_undefined(_array_elem(list, i));
                }
            }

            val_set_array(result, (intptr_t) list);
            return;
        }
    }

    val_set_undefined(result);
}

static void device_get_elem(cupkee_device_t *dev, env_t *env, int offset, val_t *res)
{
    uint32_t data;

    if (offset >= 0) {
        if (cupkee_device_get(dev, offset, &data) > 0) {
            val_set_number(res, data);
        } else {
            *res = VAL_UNDEFINED;
        }
    } else {
        device_get_all(dev, env, res);
    }
}

static int device_read_2_buffer(cupkee_device_t *dev, env_t *env, int n, val_t *buf)
{
    type_buffer_t *b;
    int err;

    b = buffer_create(env, n);
    if (!b) {
        err = -CUPKEE_ENOMEM;
    } else {
        err = cupkee_device_read(dev, n, b->buf);
    }

    if (err >= 0) {
        val_set_buffer(buf, b);
    }

    return err;
}

static int device_event_handle_set(cupkee_device_t *dev, int event, val_t *cb)
{
    device_handle_set_t *set = (device_handle_set_t *)&dev->handle_param;
    uint8_t ref_id;

    ref_id = set->handles[event];
    if (ref_id) {
        val_t *ref = shell_reference_ptr(ref_id);
        if (!ref) {
            return -CUPKEE_ERROR;
        }
        *ref = *cb;
    } else {
        val_t *ref = shell_reference_create(cb);
        if (!ref) {
            return -CUPKEE_ERESOURCE;
        }
        set->handles[event] = shell_reference_id(ref);
    }

    return CUPKEE_OK;
}

static val_t *device_event_handle_get(cupkee_device_t *dev, int event)
{
    device_handle_set_t *set = (device_handle_set_t *)&dev->handle_param;

    return shell_reference_ptr(set->handles[event]);
}

static void device_event_handle_release(cupkee_device_t *dev)
{
    device_handle_set_t *set = (device_handle_set_t *)&dev->handle_param;
    int i;

    for (i = 0; i < DEVICE_EVENT_MAX; i++) {
        val_t *ref = shell_reference_ptr(set->handles[i]);

        if (ref) {
            shell_reference_release(ref);
        }
    }
}

static void device_map_data_proc(cupkee_device_t *dev, env_t *env, val_t *handle)
{
    val_t info;

    device_get_all(dev, env, &info);

    shell_do_callback(env, handle, 1, &info);
}

static void device_data_proc(cupkee_device_t *dev, env_t *env, val_t *handle)
{
    size_t n;

    if (0 == cupkee_device_io_cached(dev, &n, NULL) && n > 0) {
        int err;
        val_t data;

        err = device_read_2_buffer(dev, env, n, &data);
        if (err < 0) {
            shell_do_callback_error(env, handle, err);
        } else {
            shell_do_callback(env, handle, 1, &data);
        }
    }
}

static void device_event_handle_wrap(cupkee_device_t *dev, uint8_t code, intptr_t param)
{
    env_t *env = cupkee_shell_env();
    val_t *handle = device_event_handle_get(dev, code);

    (void) param;

    if (!handle) {
        return;
    }

    if (code == DEVICE_EVENT_ERR) {
        shell_do_callback_error(env, handle, dev->error);
    } else
    if (code == DEVICE_EVENT_DATA) {
        // Todo: combine process of all type of device
        switch(dev->desc->category) {
        case DEVICE_CATEGORY_MAP:     device_map_data_proc(dev, env, handle); break;
        case DEVICE_CATEGORY_STREAM:
        case DEVICE_CATEGORY_BLOCK:   device_data_proc(dev, env, handle); break;
        default:                break;
        }
    } else
    if (code == DEVICE_EVENT_DRAIN) {
        shell_do_callback(env, handle, 0, NULL);
    } else
    if (code == DEVICE_EVENT_READY) {
        // Todo:
    } else {
        // What happen ?
    }
}

cupkee_device_t *cupkee_val2device(val_t *v)
{
    val_foreign_t *vf;

    if (val_is_foreign(v)) {
        vf = (val_foreign_t *)val_2_intptr(v);
        if (vf->op == &device_op) {
            return cupkee_device_block(vf->data);
        }
    }
    return NULL;
}

val_t cupkee_dev2val(env_t *env, cupkee_device_t *dev)
{
    if (dev) {
        dev->handle = device_event_handle_wrap;
        dev->handle_param = 0;
        return val_create(env, &device_op, cupkee_device_id(dev));
    } else {
        return VAL_UNDEFINED;
    }
}

val_t native_device_destroy(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;

    (void) env;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_FALSE;
    }

    device_event_handle_release(dev);
    if (CUPKEE_OK == cupkee_device_release(dev)) {
        return VAL_TRUE;
    } else {
        return VAL_FALSE;
    }
}

val_t native_device_config(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    val_t *which = NULL;
    val_t *setting;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_UNDEFINED;
    }

    if (val_is_number(av) || val_is_string(av)) {
        which = av++; ac--;
    }
    setting = ac > 0 ? av : NULL;

    if (setting) {
        // config set is forbidden, if device is disabled
        if (cupkee_device_is_enabled(dev)) {
            return VAL_FALSE;
        }

        return which ? cupkee_device_config_set_one(dev, env, which, setting) :
                       cupkee_device_config_set_all(dev, env, setting) ? VAL_FALSE : VAL_TRUE;
    } else {
        return which ? cupkee_device_config_get_one(dev, env, which) :
                       cupkee_device_config_get_all(dev);
    }
}

static inline int native_take_arg_device(int *ac, val_t **av, cupkee_device_t **dev)
{
    if ((*ac)) {
        cupkee_device_t *d = cupkee_val2device(*av);
        if (d) {
            (*ac) --; (*av) ++;
        }
        *dev = d;
        return 0;
    } else {
        return -1;
    }
}

static int native_take_arg_query(int *ac, val_t **av, void **buf, int *want)
{
    int size;
    uint8_t *ptr;
    int c = *ac;
    val_t *v = *av;

    if (c && (ptr = cupkee_val2data(v, &size))) {
        if (!(*buf = cupkee_buffer_create(size, (void *)ptr))) {
            // no memory
            return -1;
        }
        c--; v++;
    } else {
        int i = 0;
        while (i < c && val_is_number(v + i)) {
            i++;
        }
        size = i - 1;

        if (size > 0) {
            if (!(*buf = cupkee_buffer_alloc(size))) {
                // no memory
                return -1;
            }
            for (i = 0; i < size; i++) {
                cupkee_buffer_push(*buf, val_2_integer(v + i));
            }

            c -= size;
            v += size;
        } else {
            *buf = NULL;
        }
    }

    if (c && val_is_number(v)) {
        *want = val_2_integer(v);
        c--; v++;
    } else {
        *want = 0;
    }

    *ac = c;
    *av = v;

    return 0;
}

static int device_response2buffer(void *response, val_t *obj)
{
    int len;
    if (response && 0 < (len = cupkee_buffer_length(response))) {
        type_buffer_t *b = buffer_create(cupkee_shell_env(), len);

        if (!b) {
            return -CUPKEE_ERESOURCE;
        }

        cupkee_buffer_take(response, len, b->buf);
        val_set_buffer(obj, b);
    } else {
        val_set_undefined(obj);
    }

    return 0;
}

static void device_response_handle(void *d, int state, intptr_t param)
{
    cupkee_device_t *dev = (cupkee_device_t *) d;
    void *res = cupkee_device_response_take(dev);

    if (param) {
        val_t *fn = (val_t *) param;
        int   ac;
        val_t av[2];

        if (state || (state = device_response2buffer(res, &av[1]))) {
            ac = 1;
            val_set_number(av, state);
        } else {
            ac = 2;
            val_set_undefined(av);
        }
        cupkee_execute_function(fn, ac, av);

        shell_reference_release(fn);
    }
    if (res) {
        cupkee_buffer_release(res);
    }
}

val_t native_device_query(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    void *req;
    int want;
    intptr_t cb;

    (void) env;

    if (native_take_arg_device(&ac, &av, &dev)) {
        return VAL_UNDEFINED;
    }

    if (native_take_arg_query(&ac, &av, &req, &want)) {
        return VAL_FALSE;
    };

    if (ac && val_is_function(av)) {
        val_t *ref = shell_reference_create(av);

        if (!ref) {
            return VAL_FALSE;
        }
        cb = (intptr_t) ref;
    } else {
        cb = 0;
    }

    if (cupkee_device_query2(dev, req, want, device_response_handle, cb)) {
        if (cb) {
            shell_reference_release((val_t *)cb);
        }
        return VAL_FALSE;
    }
    return VAL_TRUE;
}

val_t native_device_get(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    val_t result;
    int offset;

    if (ac && (dev= cupkee_val2device(av))) {
        av++; ac--;
    } else {
        return VAL_UNDEFINED;
    }

    if (ac && val_is_number(av)) {
        offset = val_2_integer(av);
    } else {
        offset = -1;
    }

    device_get_elem(dev, env, offset, &result);
    return result;
}

val_t native_device_set(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    int offset, err = 0;
    uint32_t data;

    (void) env;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_FALSE;
    }

    if (ac < 1) {
        // Do nothing
        return VAL_TRUE;
    } else
    if (ac == 1) {
        if (val_is_number(av)) {
            offset = -1; // set all
            data = val_2_integer(av);
        } else {
            err = -CUPKEE_EINVAL;
        }
    } else {
        if (val_is_number(av) && val_is_number(av + 1)) {
            offset = val_2_integer(av);
            data = val_2_integer(av + 1);
        } else {
            err = -CUPKEE_EINVAL;
        }
    }

    if (!err) {
        err = cupkee_device_set(dev, offset, data);
    }

    return err > 0 ? VAL_TRUE : VAL_FALSE;
}

val_t native_device_write(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    int size, offset = 0, n = 0;
    int err = 0;
    void  *ptr;
    val_t *data;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_FALSE;
    }

    if (ac < 1 || (ptr = cupkee_val2data(av, &size)) == NULL) {
        err = -CUPKEE_EINVAL;
    } else {
        data = av++; ac--;

        if (ac && val_is_number(av)) {
            if (ac > 1 && val_is_number(av + 1)) {
                offset = val_2_integer(av);
                n = val_2_integer(av + 1);
                ac--; av++;
            } else {
                n = val_2_integer(av);
            }
            ac--; av++;
        } else {
            n = size;
        }

        if (offset < 0 || n < 0) {
            err = -CUPKEE_EINVAL;
        }
    }

    if (err) {
        if (ac) {
            shell_do_callback_error(env, av, err);
        }
        return VAL_FALSE;
    }

    if (n > 0 && offset < size) {
        if (offset + n > size) {
            n = size - offset;
        }
        n = cupkee_device_write(dev, n, ptr + offset);
    } else {
        n = 0;
    }

    if (ac) {
        val_t args[3];

        val_set_undefined(args);
        args[1] = *data;
        val_set_number(args + 2, n);

        shell_do_callback(env, av, 3, args);
    }

    return VAL_TRUE;
}

val_t native_device_read(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    val_t args[2];
    int want, err = 0;
    size_t in;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_FALSE;
    }

    if (0 != cupkee_device_io_cached(dev, &in, NULL)) {
        in = 0;
    }

    if (ac && val_is_number(av)) {
        want = val_2_integer(av);
        if (want < 0) {
            want = 0;
        }
        ac--; av++;
    } else {
        want = in;
    }

    if (want > 0) {
        size_t n = (size_t) want;

        if (n > in) {
            cupkee_device_read_req(dev, n - in);
            n -= in;
        }
        if (n) {
            err = device_read_2_buffer(dev, env, n, args);
            if (err < 0) {
                shell_do_callback_error(env, av, err);
            } else
            if (err > 0) {
                val_set_undefined(&args[0]);
                shell_do_callback(env, av, 2, args);
            }
        }
    }

    return VAL_TRUE;
}

val_t native_device_listen(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    val_t *callback;
    int event_id;

    (void) env;

    if (ac >= 3 && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_FALSE;
    }

    event_id = shell_val_id(av, DEVICE_EVENT_MAX, device_event_names);
    callback = av + 1;
    if (event_id < 0 || event_id > DEVICE_EVENT_MAX || !val_is_function(callback)) {
        return VAL_FALSE;
    }

    if (CUPKEE_OK == device_event_handle_set(dev, event_id, callback)) {
        return VAL_TRUE;
    } else {
        return VAL_FALSE;
    }
}

val_t native_device_ignore(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    int event_id;

    (void) env;

    if (ac >= 2 && (dev = cupkee_val2device(av)) != NULL) {
        av++; ac--;
    } else {
        return VAL_UNDEFINED;
    }

    event_id = shell_val_id(av, DEVICE_EVENT_MAX, device_event_names);
    if (0 <= event_id && event_id < DEVICE_EVENT_MAX) {
        device_handle_set_t *set = (device_handle_set_t *)&dev->handle_param;
        uint8_t ref_id = set->handles[event_id];

        set->handles[event_id] = 0;
        shell_reference_release(shell_reference_ptr(ref_id));

        return VAL_TRUE;
    } else {
        return VAL_FALSE;
    }
}

val_t native_device_is_enabled(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;

    (void) env;

    if (ac == 0 || (dev = cupkee_val2device(av)) == NULL) {
        return VAL_UNDEFINED;
    } else {
        return cupkee_device_is_enabled(dev) ? VAL_TRUE : VAL_FALSE;
    }
}

val_t native_device_enable(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;
    val_t *setting, *hnd;
    int err = 0;

    (void) env;

    if (ac && (dev = cupkee_val2device(av)) != NULL) {
        hnd = av++; ac--;
    } else {
        return VAL_UNDEFINED;
    }

    setting = ac > 0 ? av : NULL;
    if (setting && val_is_object(setting)) {
        if (cupkee_device_is_enabled(dev)) {
            err = -CUPKEE_EENABLED;
        } else {
            err = cupkee_device_config_set_all(dev, env, setting);
        }
        ac--; av++;
    }

    if (!err) {
        err = cupkee_device_enable(dev);
    }

    if (ac && val_is_function(av)) {
        val_t args[2];

        args[0] = err ? val_mk_number(err) : VAL_UNDEFINED;
        args[1] = *hnd;

        shell_do_callback(env, av, 2, args);
    }

    return (err == CUPKEE_OK) ? VAL_TRUE : VAL_FALSE;
}

val_t native_device_disable(env_t *env, int ac, val_t *av)
{
    cupkee_device_t *dev;

    (void) env;

    if (ac == 0 || (dev = cupkee_val2device(av)) == NULL) {
        return VAL_UNDEFINED;
    }
    cupkee_device_disable(dev);

    return  cupkee_device_is_enabled(dev) ? VAL_FALSE : VAL_TRUE;
}

#endif

