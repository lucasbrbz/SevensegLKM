from pathlib import Path
import os

# Importa o módulo Tkinter para criar a interface gráfica
from tkinter import Tk, Canvas, Button, PhotoImage

# Declaração das constantes
# Caminho para a pasta que contém as imagens dos botões
ASSETS_PATH = Path(os.path.join(os.path.dirname(__file__), "assets/frame0"))
# Caminho no sistema para o device criado pelo driver do display de 7 segmentos
DEVICE_FILE = '/dev/sevenseg'
# Palavras binárias que representam cada segmento individualmente (de A a G respectivamente)
SEGMENT_BITS = [0b10000000, 0b01000000, 0b00100000, 0b00010000, 0b00001000, 0b00000100, 0b00000010]

def relative_to_assets(path: str) -> Path:
    """
    Retorna o caminho completo para um arquivo na pasta
    de assets (apenas configuração da interface gráfica)

    Parâmetros:
    path (str): Nome do arquivo na pasta de assets.

    Retorno:
    Path: Caminho completo para o arquivo.
    """
    return ASSETS_PATH / Path(path)

def send_to_device(data: int):
    """
    Envia um valor binário para o dispositivo do display
    de 7 segmentos.

    Parâmetros:
    data (int): Valor inteiro representando o estado binário dos segmentos.
    """
    binary_string = bin(data)[2:].zfill(8)  # Converte o valor int 'data' para uma string binária de 8 bits, para alinhar com o que foi definido no driver
                                            # A função bin(data) converte o inteiro para binário ('0b101'), e [2:] remove o prefixo '0b'
                                            # A função .zfill(8) adiciona zeros à esquerda para garantir que a string final tenha 8 caracteres
    try:
        # Abre o dispositivo e escreve a string binária exatamente como faríamos com o comando 'echo' no Terminal
        with open(DEVICE_FILE, 'w') as device:
            device.write(binary_string)
    except IOError as e:
        print(f"Erro ao escrever em {DEVICE_FILE}: {e}")

def read_from_device() -> int:
    """
    Lê o estado atual do dispositivo do display de 7 segmentos.

    Retorno:
    int: Valor inteiro representando o estado binário dos segmentos.
    """
    try:
        # Lê 8 bits do dispositivo e converte de volta para um número inteiro
        with open(DEVICE_FILE, 'rb') as device:
            binary_string = device.read(8).decode().rstrip('\x00') + '0'    # Lê 8 bytes do dispositivo e converte para string com decode(), remove caracteres nulos à direita com rstrip().
                                                                            # Adiciona '0' no final para garantir que a string final tenha 8 bits, evitando problemas de leitura incompleta.
            return int(binary_string, 2)
    except IOError as e:
        print(f"Erro ao ler de {DEVICE_FILE}: {e}")
        # Caso não seja possível ler do dispositivo, é melhor nem continuar rodando o programa
        exit()

def toggle_segment(segment_index: int, button: Button, images: dict):
    """
    Alterna o estado de um segmento do display e atualiza o botão correspondente na GUI.

    Parâmetros:
    segment_index (int): Índice do segmento a ser alternado (1 a 7).
    button (Button): Instância do botão que representa o segmento na GUI.
    images (dict): Dicionário para armazenar e gerenciar imagens dos botões.
    """
    global segment_state
    segment_bit = SEGMENT_BITS[segment_index - 1]       # Obtém a palavra binária relativa ao acendimento de um segmento específico
    
    # Alterna diretamente o bit correspondente usando lógica XOR
    segment_state ^= segment_bit
    # Define a imagem apropriada com base no novo estado
    img_file = f"button_{segment_index}_red.png" if segment_state & segment_bit else f"button_{segment_index}.png"

    # Atualiza a imagem do botão na interface gráfica
    images[segment_index] = PhotoImage(file=relative_to_assets(img_file))
    button.config(image=images[segment_index])
    # Envia o estado atualizado para o dispositivo de 7 segmentos
    send_to_device(segment_state)

def initialize_gui(buttons: list, images: dict):
    """
    Inicializa o estado da GUI com base no estado atual do dispositivo.

    Parâmetros:
    buttons (list): Lista de botões que representam os segmentos na GUI.
    images (dict): Dicionário para armazenar e gerenciar imagens dos botões.
    """
    global segment_state
    for i, button in enumerate(buttons, start=1):
        segment_bit = SEGMENT_BITS[i - 1]
        # Define a imagem do botão de acordo com o estado inicial do segmento
        img_file = f"button_{i}_red.png" if segment_state & segment_bit else f"button_{i}.png"
        images[i] = PhotoImage(file=relative_to_assets(img_file))
        button.config(image=images[i])

def create_button(image_file: str, x: int, y: int, width: int, height: int, segment_index: int, images: dict) -> Button:
    """
    Cria e retorna um botão de controle de segmento.

    Parâmetros:
    image_file (str): Nome do arquivo de imagem do botão.
    x (int): Coordenada X para posicionamento do botão.
    y (int): Coordenada Y para posicionamento do botão.
    width (int): Largura do botão.
    height (int): Altura do botão.
    segment_index (int): Índice do segmento que o botão controla (1 a 7).
    images (dict): Dicionário para armazenar e gerenciar imagens dos botões.

    Retorno:
    Button: Instância do botão criado.
    """
    # Carrega a imagem associada ao botão
    image = PhotoImage(file=relative_to_assets(image_file))
    # Cria um botão com a imagem e uma função para alternar o estado do segmento quando clicado
    button = Button(
        image=image,
        borderwidth=0,
        highlightthickness=0,
        command=lambda: toggle_segment(segment_index=segment_index, button=button, images=images),
        relief="flat"
    )
    button.image = image                                # Armazena a imagem no botão para evitar que seja descartada pelo garbage collector
    button.place(x=x, y=y, width=width, height=height)  # Define a posição e tamanho do botão na janela
    return button

# Configuração da Janela Principal
window = Tk()                           # Instancia a classe Tk() da biblioteca Tkinter
window.geometry("234x255+30+30")        # Define o tamanho e a posição inicial da janela
window.configure(bg="#FFFFFF")          # Define a cor de fundo da janela
window.title("Display 7 Segmentos")     # Define o título da janela

# Cria um canvas (área de desenho) para adicionar elementos gráficos
canvas = Canvas(
    window,
    bg="#FFFFFF",
    height=255,
    width=234,
    bd=0,
    highlightthickness=0,
    relief="ridge"
)
# Posiciona o canvas na janela da aplicação
canvas.place(x=0, y=0)

# Leitura do estado inicial do display e configuração dos botões
segment_state = read_from_device()  # Armazena o estado atual dos segmentos no momento de inicialização da interface
images = {}                         # Dicionário para armazenar as imagens associadas aos botões

# Cria os botões que representam os segmentos do display de 7 segmentos
buttons = [
    create_button("button_1.png", 88, 83, 59, 11, 1, images),
    create_button("button_2.png", 155, 94, 11, 59, 2, images),
    create_button("button_3.png", 155, 164, 11, 59, 3, images),
    create_button("button_4.png", 88, 223, 59, 11, 4, images),
    create_button("button_5.png", 68, 164, 11, 59, 5, images),
    create_button("button_6.png", 68, 94, 11, 59, 6, images),
    create_button("button_7.png", 88, 153, 59, 11, 7, images)
]

# Elementos Visuais Fixos
canvas.create_rectangle(0, 0, 234, 63, fill="#1B4586", outline="")
canvas.create_text(25, 24, anchor="nw", text="Display 7 Segmentos", fill="#FFFFFF", font=("FiraCode Bold", 16 * -1))

# Inicializa os botões da interface com base no estado atual do dispositivo
initialize_gui(buttons, images)

# Inicia o loop principal da interface gráfica (mantém a janela aberta e interativa)
window.resizable(False, False)
window.mainloop()
