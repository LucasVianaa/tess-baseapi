#include <vector>
#include <iostream>
#include <string>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <tuple>
#include <fstream>
#include <locale>              
#include <memory>              
#include <sstream> 
#include <cmath>

#include <argp.h>
using namespace std;




tesseract::PageSegMode getPSM(int psm){
  
  switch (psm)
  {
  case 0:
    return tesseract::PSM_OSD_ONLY;
    break;
  case 1:
    return tesseract::PSM_AUTO_OSD;
    break;
  case 2:
    return tesseract::PSM_AUTO_ONLY;
    break;
  case 3:
    return tesseract::PSM_AUTO;
    break;
  case 4:
    return tesseract::PSM_SINGLE_COLUMN;
    break;
  case 5:
    return tesseract::PSM_SINGLE_BLOCK_VERT_TEXT;
    break;
  case 6:
    return tesseract::PSM_SINGLE_BLOCK;
    break;
  case 7:
    return tesseract::PSM_SINGLE_LINE;
    break;
  case 8:
    return tesseract::PSM_SINGLE_WORD;
    break;
  case 9:
    return tesseract::PSM_CIRCLE_WORD;
    break;
  case 10:
    return tesseract::PSM_SINGLE_CHAR;
    break;
  case 11:
    return tesseract::PSM_SPARSE_TEXT;
    break;
  case 12:
    return tesseract::PSM_SPARSE_TEXT_OSD;
    break;
  case 13:
    return tesseract::PSM_RAW_LINE;
    break;
  
  default:
    break;
  }
  return tesseract::PSM_AUTO;
}

//Compilação
//(Opção 1 | Instalação padrão do tesseract) sudo g++ -o program main.cpp -llept -ltesseract
//(Opção 2 | Tesseract compilado) sudo g++ -o program main.cpp -I/usr/local/include/tesseract -L/usr/local/lib -llept -ltesseract


//Execução: sudo ./program teste.png teste.xml lstm config.txt 3 por+eng
//(OBS: arquivo de entrada deve ser uma imagem)
int main(int argc, char* argv[])
{
    if (argc < 7) {
        std::cerr << "Usage: " << argv[0] << " PATH/TO/INPUT/FILENAME" << " /PATH/TO/OUTPUT/FILENAME" << " MODE(LSTM or Legacy)" << " PATH/TO/CONFIG/FILENAME" << " PSM(number)" << " Languages" << endl;
        return 1;
    }
   /*
    std::vector<std::string> pars_vec;
    pars_vec.push_back("load_system_dawg");
    pars_vec.push_back("load_freq_dawg");
    pars_vec.push_back("load_unambig_dawg");
    pars_vec.push_back("load_punc_dawg");
    pars_vec.push_back("load_number_dawg");
    pars_vec.push_back("load_bigram_dawg");
    */

    /*
    std::vector<std::string> pars_values;
    pars_values.push_back("0");
    pars_values.push_back("0");
    pars_values.push_back("0");
    pars_values.push_back("0");
    pars_values.push_back("0");
    pars_values.push_back("0");
    */
   std::string outFile = std::string(argv[2]);
   std::ofstream ofs;
   ofs.open(outFile, std::ofstream::out | std::ofstream::trunc);
   ofs.close();
   tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
   if(strcmp(argv[3], "legacy") == 0){
     if (api->Init(NULL, argv[6], tesseract::OcrEngineMode::OEM_TESSERACT_ONLY)) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
   }else{
     if (api->Init(NULL, argv[6], tesseract::OcrEngineMode::OEM_LSTM_ONLY)) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
   }
   api->ReadConfigFile(argv[4]);
    int lcnt = 1, bcnt = 1, pcnt = 1, wcnt = 1, scnt = 1, tcnt = 1, ccnt = 1;
    int page_id = 1; // hOCR uses 1-based page numbers.
    bool para_is_ltr = true;       // Default direction is LTR
    const char *paragraph_lang = nullptr;
    bool font_info;
    api->GetBoolVariable("hocr_font_info", &font_info);
    bool hocr_boxes;
    api->GetBoolVariable("hocr_char_boxes", &hocr_boxes);

    std::string img = std::string(argv[1]);
    // Open input image with leptonica library
    Pix *image = pixRead(img.c_str());
    api->SetPageSegMode(getPSM(std::stoi(argv[5])));
    api->SetImage(image);
    char* outFull = api->GetUTF8Text();
    
    // Destroy used object and release memory
    api->End();
    delete api;
    delete [] outFull;
    pixDestroy(&image);

    return 0;
}