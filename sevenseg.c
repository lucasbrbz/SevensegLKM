#include <linux/init.h>           // Macros para funções de inicialização e finalização do módulo
#include <linux/module.h>         // Macros essenciais para criar módulos de Kernel
#include <linux/kernel.h>         // Funções de log do Kernel, como printk()
#include <linux/gpio.h>           // API para controle de GPIOs (pinos de entrada/saída)
#include <linux/fs.h>             // Funções relacionadas ao sistema de arquivos
#include <linux/uaccess.h>        // Funções para transferir dados entre o espaço do usuário e o Kernel
#include <linux/cdev.h>           // Estruturas e funções para registrar dispositivos de caractere
#include <linux/device.h>         // Estruturas para gerenciar dispositivos no Kernel

#define DEVICE_NAME "sevenseg"    // Nome do dispositivo, aparecerá em /dev/sevenseg
#define CLASS_NAME "sevenseg"     // Nome da classe de dispositivos, serve para agrupar dispositivos similares

MODULE_LICENSE("GPL");            // Define a licença do módulo como GNU Public License - licença para código aberto sobre a qual podemos distribuir nosso código, permitindo cópia e alteração
MODULE_AUTHOR("Lucas Barboza");
MODULE_DESCRIPTION("Driver para controlar um display de 7 segmentos");
MODULE_VERSION("1.0");

/**
 * Estruturas de dados utilizadas pelo Kernel para criar
 * um character device (chardev)
 */
static int major_number;                            // Número major do dispositivo (ID utilizado pelo sistema)
static struct class* seven_segment_class = NULL;    // Estrutura que representa a classe do dispositivo
static struct device* seven_segment_device = NULL;  // Estrutura que representa o dispositivo
static struct cdev seven_segment_cdev;              // Estrutura de caractere do dispositivo (cdev)

/**
 * Definição dos pinos GPIO conectados ao display de 7 segmentos.
 * Estes pinos GPIO foram selecionados aleatoriamente, lembre-se
 * que temos um barramento de 40 pinos configuráveis à nossa disposição
 */
#define PIN_A 17
#define PIN_B 18
#define PIN_C 27
#define PIN_D 22
#define PIN_E 23
#define PIN_F 24
#define PIN_G 25

/**
 * Definição do tamanho máximo da nossa string binária, para este display temos
 * sete caracteres (um para cada segmento) + o terminador de string '\0', mas
 * poderemos atualizar este código futuramente para suportar outros tipos de display
 */
#define MAX_BUF_SIZE 8

/**
 * Aqui vamos criar um vetor com os números dos pinos selecionados
 * para automatizar a manipulação. Também fazemos um cálculo simples
 * para descobrir o tamanho do vetor automaticamente (a linguagem C
 * não possui um método simples para isso como outras linguagens).
 * Isso nos permitirá adicionar mais pinos ao vetor futuramente sem
 * nos preocuparmos em alterar muitas coisas no código
 */
static unsigned int gpio_pins[] = {PIN_A, PIN_B, PIN_C, PIN_D, PIN_E, PIN_F, PIN_G};
static int number_of_pins = sizeof(gpio_pins) / sizeof(gpio_pins[0]);   // Dividimos o tamanho do vetor inteiro pelo tamanho de 1 de seus elementos, ou seja, 7 / 1 = 7

/**
 * Função chamada quando o dispositivo é aberto
 * (quando vai trocar dados - lembrar do fopen() da linguagem C)
 */
static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "sevenseg: character device aberto\n");
    return 0; // Retorna 0 para indicar sucesso
}

/**
 * Função chamada quando o dispositivo é fechado
 * (quando já terminou de trocar dados - lembrar do fclose() da linguagem C)
 */
static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "sevenseg: character device fechado\n");
    return 0; // Retorna 0 para indicar sucesso
}

/**
 * Função chamada quando o dispositivo recebe dados a partir
 * do espaço do usuário (lembrar do fwrite() da linguagem C)
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    char message[MAX_BUF_SIZE] = {0}; // String para armazenar a mensagem binária recebida do usuário (7 caracteres + '\0')

    // Verificamos se há dados para serem escritos ou não baseado no tamanho do buffer recebido do userspace
    if (len > 0) {
        if (copy_from_user(message, buffer, len < MAX_BUF_SIZE ? len : MAX_BUF_SIZE - 1)) {           // Função que copia os dados do espaço do usuário para o Kernel (de buffer -> message)
            printk(KERN_ERR "sevenseg: falha na recepcao de dados\n");      // Aqui estamos truncando o tamanho da string recebida em no máximo 7 caracteres - proteção contra buffer overflow
            return -EFAULT;                                                 // Retorna erro se a cópia falhar
        }
        for (int i = 0; i < number_of_pins && message[i] != '\0'; i++) {    // Vamos ler a string 'message' vinda do espaco do usuário caractere por caractere até encontrar o null terminator
            gpio_set_value(gpio_pins[i], message[i] == '1');                // Se o caractere lido for '1' (char, e nao int), a condição resultará em TRUE, que equivale ao número 1 na linguagem C
        }                                                                   // Esta forma serve apenas para evitar um if/else extenso, o que tornaria o código mais difícil de ler :)
        printk(KERN_INFO "sevenseg: recebeu %zu caracteres do usuario\n", len);
    }
    return len; // Retorna o número de bytes escritos (obrigatório)
}

/**
 * Função chamada quando os dados registrados no dispositivo são lidos
 * e enviados para o espaço do usuário (lembrar do fread() da linguagem C)
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    char segment_states[MAX_BUF_SIZE]; // String para armazenar os estados dos pinos GPIO (referentes a cada segmento do display)
    int error_count;

    // Se já leu o arquivo uma vez durante esta chamada, retorna 0 para indicar que não há mais dados a serem lidos
    if (*offset > 0) {
        return 0;
    }

    for (int i = 0; i < number_of_pins; i++) {                          // Vamos coletar os estados atuais de cada pino GPIO (0 ou 1 em int)
        segment_states[i] = gpio_get_value(gpio_pins[i]) ? '1' : '0';   // e montar uma string binária adicionando '0' e '1' na forma de char
    }
    segment_states[number_of_pins] = '\0';                              // Temos que adicionar o terminador de string (null terminator)

    // Copia a string binária com o estado dos segmentos de volta para o espaço do usuário (de segment_states -> buffer)
    error_count = copy_to_user(buffer, segment_states, number_of_pins + 1);

    // Verifica se a cópia foi bem-sucedida
    if (error_count == 0) {
        printk(KERN_INFO "sevenseg: enviou %d caracteres para o usuario\n", number_of_pins);
        *offset += number_of_pins + 1; // Atualiza o offset para evitar leituras repetidas (lembrar do fseek() da linguagem C)
        return number_of_pins + 1;     // Retorna o número de bytes lidos (obrigatório)
    } else {
        printk(KERN_INFO "sevenseg: falha no envio de dados\n", error_count);
        return -EFAULT; // Retorna erro se a cópia falhar
    }
}

/**
 * Estrutura obrigatória que define as operações de arquivo do dispositivo (open, read, write, release)
 */
static struct file_operations fops = {
    .open = dev_open,
    .write = dev_write,
    .read = dev_read,
    .release = dev_release,
};

/**
 * Função chamada na inicialização do módulo (quando o módulo
 * é carregado através do comando insmod no Terminal)
 */
static int __init seven_segment_init(void) {
    int result;  // Variável para armazenar resultados de funções
    dev_t dev;   // Estrutura que armazena o major e minor number do dispositivo

    printk(KERN_INFO "sevenseg: inicializando o LKM para o display de 7 segmentos\n");

    // Solicita ao Kernel e configura os pinos GPIO como saídas (referente a cada segmento do display)
    for (int i = 0; i < number_of_pins; i++) {
        if (gpio_request(gpio_pins[i], "sysfs")) {  // Solicita a permissão para utilizar o pino GPIO
            printk(KERN_ALERT "sevenseg: falha na requisicao do pino GPIO %d\n", gpio_pins[i]);

            for (int j = 0; j < i; j++) {
                gpio_free(gpio_pins[j]);            // Libera os GPIOs já solicitados em caso de falha
            }
            return -1;                              // Retorna erro
        }
        gpio_direction_output(gpio_pins[i], 0);     // Configura o pino como saída e define o estado inicial como baixo (0)
        gpio_export(gpio_pins[i], false);           // Exporta para o sistema sysfs do Linux, permitindo o acesso pelo usuário
    }

    // Alocamos um major number dinâmico para o dispositivo de caractere (ID obrigatório para que o Kernel identifique o driver)
    result = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (result < 0) {
        printk(KERN_ALERT "sevenseg: falha ao registrar um major number\n");
        return result;
    }
    major_number = MAJOR(dev);
    printk(KERN_INFO "sevenseg: registrado corretamente com major number %d\n", major_number);

    // Criamos uma classe de dispositivo no Kernel para facilitar a criação e o gerenciamento do dispositivo (obrigatório)
    seven_segment_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(seven_segment_class)) {
        unregister_chrdev_region(dev, 1);
        printk(KERN_ALERT "sevenseg: falha ao registrar classe do device\n");
        return PTR_ERR(seven_segment_class);
    }
    printk(KERN_INFO "sevenseg: registrada corretamente a classe do device\n");

    // Criamos um dispositivo (ou "arquivo") localizado em /dev/sevenseg
    seven_segment_device = device_create(seven_segment_class, NULL, dev, NULL, DEVICE_NAME);
    if (IS_ERR(seven_segment_device)) {
        class_destroy(seven_segment_class);
        unregister_chrdev_region(dev, 1);
        printk(KERN_ALERT "sevenseg: falha ao criar o device\n");
        return PTR_ERR(seven_segment_device);
    }
    printk(KERN_INFO "sevenseg: device criado corretamente\n");

    // Inicializamos a estrutura cdev e adicionamos o dispositivo ao sistema
    cdev_init(&seven_segment_cdev, &fops);
    seven_segment_cdev.owner = THIS_MODULE;
    result = cdev_add(&seven_segment_cdev, dev, 1);
    if (result < 0) {
        device_destroy(seven_segment_class, dev);
        class_destroy(seven_segment_class);
        unregister_chrdev_region(dev, 1);
        printk(KERN_ALERT "sevenseg: falha ao adicionar o cdev\n");
        return result;
    }

    return 0; // Sucesso na inicialização do módulo
}

/**
 * Função chamada na remoção do módulo (quando o módulo
 * é descarregado através do comando rmmod no Terminal)
 * 
 * OBS.: também conhecida como função de cleanup ou limpeza! Importante observar
 * que os passos executados na função de remoção são exatamente o inverso dos
 * passos executados na inicialização, e na ordem contrária para evitar que algum
 * passo seja esquecido e cause problemas com gerenciamento de memória!!!
 */
static void __exit seven_segment_exit(void) {
    dev_t dev = MKDEV(major_number, 0);         // Aqui utilizamos o major number (ID) para localizar e coletar as informações
                                                // do nosso driver que serão utilizadas durante a limpeza da nossa "bagunça"

    cdev_del(&seven_segment_cdev);              // Removemos o dispositivo de caractere do Kernel
    device_destroy(seven_segment_class, dev);   // Removemos o arquivo de /dev
    class_unregister(seven_segment_class);      // Desregistramos a classe do dispositivo
    class_destroy(seven_segment_class);         // Destruímos a classe do dispositivo
    unregister_chrdev_region(dev, 1);           // Liberamos o major number para que outros dispositivos possam utilizar

    // Liberamos e desconfiguramos os pinos GPIO usados
    for (int i = 0; i < number_of_pins; i++) {
        gpio_set_value(gpio_pins[i], 0);        // Desligamos os pinos (nível lógico baixo)
        gpio_unexport(gpio_pins[i]);            // Removemos do sistema sysfs do Linux
        gpio_free(gpio_pins[i]);                // Retornamos o controle do GPIO para o Kernel
    }

    printk(KERN_INFO "sevenseg: encerrando...\n");
}

/**
 * Após a compilação, o Kernel saberá que este código é um módulo de Kernel. Como todo
 * módulo deve obrigatoriamente possuir um init e um exit, usamos os marcadores a seguir
 * para sinalizar ao Kernel quais serão essas funções por meio de ponteiros de função
 */
module_init(seven_segment_init);
module_exit(seven_segment_exit);
