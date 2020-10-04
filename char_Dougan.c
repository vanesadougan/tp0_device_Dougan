#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

int init_module(void);     //Función de entrada al módulo se encarga de decirle al Kernel que funcionalidad provee y configura el kernel para que funcione.
void cleanup_module(void); // Función de salida para los módulos, se encarga de desasignar la funcionalidad que la funcion init registró.
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define BUF_LEN 100
#define DEVICE_NAME "DOUGAN"

static int MajorNumber; //Dice que driver es usado para acceder al hardware. Cada driver un único major number.
static int Device_Open = 0;
static char msg[BUF_LEN];
static char *msg_print;

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = device_write,
    .open = device_open,
    .read = device_read,
    .release = device_release};

/*
 * Función para inicializar el modulo
 */
int init_module(void)
{
    MajorNumber = register_chrdev(0, DEVICE_NAME, &fops); //Se registra el Dispositivo pasandole 0 como major (que significa que debe asignarlo dinámicamente)
    //, nombre del dispositivo y el puntero a la tabla File_operations definida en Struc.

    if (MajorNumber < 0) //Si se obtiene un número negativo al registrar el dispositivo, significa que ha fallado.
    {
        printk(KERN_ALERT "El registro del char device fallo%d\n", MajorNumber);
        return MajorNumber;
    }

    printk(KERN_INFO "Se genero el  major number %d.", MajorNumber);
    printk(KERN_INFO "Ahora se debera crear un dev_file con \n");
    printk(KERN_INFO "sudo rm /dev/%s\n", DEVICE_NAME);
    printk(KERN_INFO "sudo mknod /dev/%s c %d 0\n", DEVICE_NAME, MajorNumber);
    printk(KERN_INFO "sudo chmod 666 /dev/%s\n", DEVICE_NAME);

    return SUCCESS;
}

// Función encargada de desregistrar las funciones que registró init.
void cleanup_module(void)
{
    unregister_chrdev(MajorNumber, DEVICE_NAME);
    printk(KERN_INFO "El disposiivo fue desregistrado correctamente\n");
}

//Apertura del archivo del dispositivo.

static int device_open(struct inode *inode, struct file *filp)
{
    if (Device_Open)
        return -EBUSY;

    Device_Open++;
    msg_print = msg;
    try_module_get(THIS_MODULE); //Para operar con FS.

    return SUCCESS;
}

/*
 * Cierre del archivo del dispositivo.
 */
static int device_release(struct inode *inode, struct file *filp)
{
    Device_Open--;
    module_put(THIS_MODULE);
    return SUCCESS;
}

static ssize_t device_read(struct file *filp,
                           char *buffer,  //Buffer de datos
                           size_t length, //Longitud de buffer.
                           loff_t *offset)
{
    int bytes_read = 0;
    if (*msg_print == 0)
        return 0;
    //saco del buffer y lo pongo en el modo usuario
    while (length && *msg_print)
    {
        put_user(*(msg_print++), buffer++); // Hay que usar put_user para copiar los datos del segmento de datos del kernel al segmento de datos del usuario.
        length--;
        bytes_read++;
    }

    return bytes_read;
}

/*
 * Función para escribir en el dispositivo.
 */
static ssize_t device_write(struct file *filp, const char *tmp, size_t length, loff_t *offset)
{

    char ch;
    int i;
    memset(msg, 0, sizeof msg);
    for (i = 0; i < length && i < BUF_LEN; i++)
        //obtengo variable del modo de usuario al modo kernel
        get_user(msg[i], tmp + i); // Lo que esta en Buffer se guarda en msg.

    //Se hace el cifrado del codigo guardado.
    for (i = 0; i < length && i < BUF_LEN; i++)
    {
        ch = msg[i];
        if (ch >= 'a' && ch <= 'z')
        {

            ch = ch + 5;

            if (ch > 'z')
            {
                ch = ch - 'z' + 'a' - 1;
            }
            msg[i] = ch;
        }
        else if (ch >= 'A' && ch <= 'Z')
        {
            ch = ch + 5;

            if (ch > 'Z')
            {
                ch = ch - 'Z' + 'A' - 1;
            }

            msg[i] = ch;
        }
    }
    return length;
}
