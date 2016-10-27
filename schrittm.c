#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_ALERT */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/fs.h>

#include <asm/io.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Nebojsa Jovicic");
MODULE_DESCRIPTION("Schrittmotor Treiber");

/* Schrittmotor adresse */
static int addr = 0x115;

module_param(addr, int, 0);
MODULE_PARM_DESC(addr, "Base Address (default=0x115)");

#define NAME "schrittm"
#define BASE (addr)

static int schrittm_major = 60;

static unsigned char TurnStepperList[2][2][8] = {
        {{0xA,0x9,0x5,0x6,0xA,0x9,0x5,0x6},{0xA,0x8,0x9,0x1,0x5,0x4,0x6,0x2}},
        {{0x6,0x5,0x9,0xA,0x6,0x5,0x9,0xA},{0x2,0x6,0x4,0x5,0x1,0x9,0x8,0xA}}
        };

char message[90] = "";
static int stepcount = 0, mode = 0, richtung = 0;
enum MODE {half = 0, full = 1};
enum RICHTUNG {vor = 0, zurueck = 1};

static ssize_t schrittm_read(struct file *file, char * buffer, size_t len, loff_t *f_pos)
{
  ssize_t msg_len;
  msg_len = strlen(message);

  put_user(message[*f_pos], buffer);

  *f_pos+=1;
  if (*f_pos > msg_len)
  {
    *f_pos = 0;
    return 0;
  }

  return 1;
}

static int step(void)
{
  if (stepcount < 8)
  {
    sprintf(message, "step:%d richtung:%d mode:%d\n", TurnStepperList[richtung][mode][++stepcount], richtung, mode);
    printk(message);
    //outb(BASE, TurnStepperList[richtung][mode][stepcount]);
  }
  else
  {
    stepcount = 0;
    sprintf(message, "step:%d richtung:%d mode:%d\n", TurnStepperList[richtung][mode][++stepcount], richtung, mode);
    printk(message);
    //outb(BASE, TurnStepperList[richtung][mode][stepcount]);
  }
  return 0;
}

static ssize_t schrittm_write(struct file * file,  const char * buffer, size_t len, loff_t *off)
{
  char data;
  get_user(data, buffer);

  switch (data)
  {
    case 'H':
      sprintf(message, "Mode: [f]ullstep. [h]alfstep\nRichtung: [v]orwaerts, [z]urueck\n");
      printk(message);
      break;
    case 'h':
      sprintf(message, "Half-Step mode initialized\n");
      printk(message);
      mode = half;
      break;
    case 'f':
      sprintf(message, "Full-Step mode initialized\n");
      printk(message);
      mode = full;
      break;
    case 'v':
      richtung = vor;
      step();
      break;
    case 'z':
      richtung = zurueck;
      step();
      break;
  }

  return 1;
}

static struct file_operations ad_fops = {
  read: schrittm_read,
  write: schrittm_write,
};

static int __init schrittm_init_module(void)
{
  int err;

  printk(KERN_NOTICE "Installing " NAME " module\n");

  err = register_chrdev(schrittm_major,NAME,&ad_fops);

  if (err < 0) {
    printk(KERN_NOTICE "unable to get major %d for " NAME " devs\nerror: %d\n", schrittm_major, -err);
    return err;
  }

  printk(KERN_NOTICE NAME ": got major %d\n", schrittm_major);

  if (request_region(BASE, 1, NAME) == 0) {
    printk(KERN_NOTICE "unable to allocate port %d(0x%x) for " NAME  " devs\n", BASE, BASE);
    unregister_chrdev(schrittm_major, NAME);
    return -ENODEV;
  }

  printk(KERN_NOTICE "schrittm module installed\n");
  return 0;
}

static void __exit schrittm_cleanup_module(void)
{
  release_region(BASE, 1);
  unregister_chrdev(schrittm_major, NAME);

  printk(KERN_NOTICE "Deinstalling " NAME " module\n");
}

module_init(schrittm_init_module);
module_exit(schrittm_cleanup_module);

