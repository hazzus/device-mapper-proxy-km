#include <linux/device-mapper.h>

#include <linux/bio.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>

#define DM_MSG_PREFIX "dmp"

struct proxy_info {
    struct dm_dev* inner;
    unsigned long read_cnt;
    unsigned long read_avg;
    unsigned long write_cnt;
    unsigned long write_avg;
    unsigned long total_avg;
};

struct kobject* stat_obj;

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv) {
    struct proxy_info *pi;

    if (argc != 1) {
        ti->error = "Wrong number of arguments";
        return -EINVAL;
    }

    DMINFO("Initializing dm proxy target for %s", argv[0]);

    pi = kmalloc(sizeof(struct proxy_info), GFP_KERNEL);
    if (pi == NULL) {
        DMERR("Error in kmalloc for proxy_info");
        ti->error = "Cannot allocate proxy information";
        return -ENOMEM;
    }
    
    if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &pi->inner)) {
        ti->error = "Device lookup failed";
        kfree(pi);
        return -EINVAL;
    }
    ti->private = pi;

    DMINFO("Target dmp %s constructed", argv[0]);

    return 0;    
}

static int dmp_map(struct dm_target* ti, struct bio* bio) {
    struct proxy_info *pi = (struct proxy_info*)ti->private;
    unsigned long long bio_size = bio_cur_bytes(bio);

    switch (bio_op(bio)) {
    case REQ_OP_READ:
        pi->read_cnt++;

        pi->read_avg += (bio_size - pi->read_avg) / pi->read_cnt;

        pi->total_avg += (bio_size - pi->total_avg) / (pi->read_cnt + pi->write_cnt);

        break;
    case REQ_OP_WRITE:
        pi->write_cnt++;

        pi->write_avg += (bio_size - pi->write_avg) / pi->write_cnt;

        pi->total_avg += (bio_size - pi->total_avg) / (pi->read_cnt + pi->write_cnt);
    }

    bio_set_dev(bio, pi->inner->bdev);
    submit_bio(bio);

    return DM_MAPIO_SUBMITTED;
}

static void dmp_dtr(struct dm_target *ti) {
    struct proxy_info *pi = (struct proxy_info*)ti->private;
    dm_put_device(ti, pi->inner);
    kfree(pi);
    DMINFO("dmp target desctructed");
}

static struct target_type dmp_target = {
    .name = "dmp",
    .version = {0, 0, 1},
    .module = THIS_MODULE,
    .ctr = dmp_ctr,
    .map = dmp_map,
    .dtr = dmp_dtr,
};

static int __init dmp_init(void) {
    int r;
    struct kobject mod_kobj = (((struct module*)(THIS_MODULE))->mkobj).kobj;
    stat_obj = kobject_create_and_add("stat", &mod_kobj);

    if (stat_obj == NULL) {
        DMERR("Failed creating stat directory");
        return -1;
    }

    DMINFO("Registering dmp target");
   
    r = dm_register_target(&dmp_target); 
    if (r < 0) {
        DMERR("Register of dmp target failed with errcode: %d", r);
    } else {
        DMINFO("Registered dmp target successfully");
    }
    
    return r;
}

static void __exit dmp_exit(void) {
    DMINFO("Unregistering dmp target");
    dm_unregister_target(&dmp_target);
    kobject_put(stat_obj);
    DMINFO("Unregistered dmp target successfully");
}

module_init(dmp_init);
module_exit(dmp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pavel Bezborodov <p.a.bezborodov@gmail.com>");
MODULE_DESCRIPTION("Device mapper proxy");
MODULE_VERSION("0.01");
