#include <linux/device-mapper.h>

#include <linux/bio.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>

#define DM_MSG_PREFIX "dmp"

struct proxy_info {
    long long read_cnt, read_avg, 
              write_cnt, write_avg, 
              total_avg;
};

static struct proxy_info stats = {
    .read_cnt = 0,
    .read_avg = 0,
    .write_cnt = 0,
    .write_avg = 0,
    .total_avg = 0,
};

static ssize_t sysfs_show(struct kobject *kobj,
                          struct kobj_attribute *attr, char *buf) {
    const char* fmt = "\
read:\n\
  reqs: %lld \n\
  avg: %lld \n\
write:\n\
  reqs: %lld \n\
  avg: %lld \n\
total:\n\
  reqs: %lld \n\
  avg: %lld \n\
"; 
    return sprintf(buf, fmt,
                   stats.read_cnt,
                   stats.read_avg,
                   stats.write_cnt,
                   stats.write_avg,
                   stats.read_cnt + stats.write_cnt,
                   stats.total_avg);
}

static struct kobj_attribute stats_attr = 
    __ATTR(op_stat, 0440, sysfs_show, NULL);

static struct kobject* stat_kobj;

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv) {
    struct dm_dev *dev;
    int reinit;
    if (argc != 2) {
        ti->error = "Wrong number of arguments";
        return -EINVAL;
    }
 
    DMINFO("Initializing dm proxy target for %s", argv[0]);
    
    if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &dev)) {
        ti->error = "Device lookup failed";
        return -EINVAL;
    }
    ti->private = dev;
    
    DMINFO("Target dmp %s constructed", argv[0]);
    
    if (!sscanf(argv[1], "%d", &reinit)) {
        ti->error = "Second argument not a number";
        return -EINVAL;
    }
    if (reinit) {
        stats.read_cnt = 0;
        stats.read_avg = 0;
        stats.write_cnt = 0;
        stats.write_avg = 0;
        stats.total_avg = 0;
    }
    return 0;    
}

static int dmp_map(struct dm_target* ti, struct bio* bio) {
    struct dm_dev *dev = (struct dm_dev*)ti->private;
    // the sector size
    long long bio_size = bio_sectors(bio) * 512;
    
    // using streaming algo to prevent overflow
    switch (bio_op(bio)) {
    case REQ_OP_READ:
        stats.read_cnt++;

        stats.read_avg += (bio_size - stats.read_avg) / stats.read_cnt;

        break;
    case REQ_OP_WRITE:
        stats.write_cnt++;

        stats.write_avg += (bio_size - stats.write_avg) / stats.write_cnt;
    }
    
    stats.total_avg += (bio_size - stats.total_avg) / (stats.read_cnt + stats.write_cnt);

    bio_set_dev(bio, dev->bdev);
    submit_bio(bio);

    return DM_MAPIO_SUBMITTED;
}

static void dmp_dtr(struct dm_target *ti) {
    struct dm_dev *dev = (struct dm_dev*)ti->private;
    dm_put_device(ti, dev);
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
    stat_kobj = kobject_create_and_add("stat", &mod_kobj);
    
    DMINFO("Creating stat and volumes file");

    if (!stat_kobj) {
        DMERR("Failed creating stat directory");
        return -ENOMEM;
    }

    if (sysfs_create_file(stat_kobj, &stats_attr.attr)) {
        sysfs_remove_file(stat_kobj, &stats_attr.attr);
        kobject_put(stat_kobj);
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
    sysfs_remove_file(stat_kobj, &stats_attr.attr);
    kobject_put(stat_kobj);
    dm_unregister_target(&dmp_target);
    DMINFO("Unregistered dmp target successfully");
}

module_init(dmp_init);
module_exit(dmp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pavel Bezborodov <p.a.bezborodov@gmail.com>");
MODULE_DESCRIPTION("Device mapper proxy");
MODULE_VERSION("0.01");
