# tess-baseapi

# Compilação
(Opção 1 | Instalação padrão do tesseract) sudo g++ -o program main.cpp -llept -ltesseract
(Opção 2 | Tesseract compilado) sudo g++ -o program main.cpp -I/usr/local/include/tesseract -L/usr/local/lib -llept -ltesseract


# Execução 
sudo ./program path/to/input/filename path/to/output/filename legacy 
(OBS: arquivo de entrada deve ser ./png e o arquivo de saída não deve conter a extensão)