#include <linux/device-mapper.h>

#include <linux/bio.h>
#include <linux/init.h>
#include <linux/module.h>

#define DM_MSG_PREFIX "dmp"

struct proxy_info {
    struct dm_dev* inner;
    int read_ops;
    int read_avg;
    int write_ops;
    int write_avg;
};

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv) {
    struct proxy_info *pi;

    if (argc != 1) {
        ti->error = "Wrong number of arguments";
        return -EINVAL;
    }

    DMINFO("Initializing dmp target %s", argv[0]);

    pi = kmalloc(sizeof(struct proxy_info), GFP_KERNEL);
    if (pi == NULL) {
        DMERR("Error in kmalloc for proxy_info");
        ti->error = "Cannot allocate proxy information";
        return -ENOMEM;
    }
    
    if (dm_get_device(ti, argv[0],
                      dm_table_get_mode(ti->table), &pi->inner)) {
        ti->error = "Device lookup failed";
        return -EINVAL;
    }

    ti->private = pi;

    DMINFO("dmp target constructed");

    return 0;    
}

static int dmp_map(struct dm_target* ti, struct bio* bio) {
    struct proxy_info *pi = (struct proxy_info*)ti->private;

    DMINFO("Something happened to dmp");
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
    printk(KERN_INFO "Registering dmp target");

    int r = dm_register_target(&dmp_target);

    if (r < 0) {
        DMERR("register of dmp target failed %d", r);
    }

    return r;
}

static void __exit dmp_exit(void) {
    printk(KERN_INFO "Goodbye, world!\n");
}

module_init(dmp_init);
module_exit(dmp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pavel Bezborodov <p.a.bezborodov@gmail.com>");
MODULE_DESCRIPTION("Device mapper proxy");
MODULE_VERSION("0.01");
