# tess-baseapi

# Compilação
(Opção 1 | Instalação padrão do tesseract) sudo g++ -o program main.cpp -llept -ltesseract <br>
(Opção 2 | Tesseract compilado) sudo g++ -o program main.cpp -I/usr/local/include/tesseract -L/usr/local/lib -llept -ltesseract <br>


# Execução 
sudo ./program teste.png teste.xml lstm config.txt 3 por+eng <br>
(OBS: arquivo de entrada deve ser .png) <br>