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

std::string HOcrEscape(const char *text) {
  std::string ret;
  const char *ptr;
  for (ptr = text; *ptr; ptr++) {
    switch (*ptr) {
      case '<':
        ret += "&lt;";
        break;
      case '>':
        ret += "&gt;";
        break;
      case '&':
        ret += "&amp;";
        break;
      case '"':
        ret += "&quot;";
        break;
      case '\'':
        ret += "&#39;";
        break;
      default:
        ret += *ptr;
    }
  }
  return ret;
}
static tesseract::Orientation GetBlockTextOrientation(const tesseract::PageIterator *it) {
  tesseract::Orientation orientation;
  tesseract::WritingDirection writing_direction;
  tesseract::TextlineOrder textline_order;
  float deskew_angle;
  it->Orientation(&orientation, &writing_direction, &textline_order, &deskew_angle);
  return orientation;
}
void AddBaselineCoordsTohOCR(const tesseract::PageIterator *it, tesseract::PageIteratorLevel level, std::stringstream &hocr_str) {
  tesseract::Orientation orientation = GetBlockTextOrientation(it);
  if (orientation != tesseract::ORIENTATION_PAGE_UP) {
    hocr_str << "; textangle " << 360 - orientation * 90;
    return;
  }

  int left, top, right, bottom;
  it->BoundingBox(level, &left, &top, &right, &bottom);

  // Try to get the baseline coordinates at this level.
  int x1, y1, x2, y2;
  if (!it->Baseline(level, &x1, &y1, &x2, &y2))
    return;
  // Following the description of this field of the hOCR spec, we convert the
  // baseline coordinates so that "the bottom left of the bounding box is the
  // origin".
  x1 -= left;
  x2 -= left;
  y1 -= bottom;
  y2 -= bottom;

  // Now fit a line through the points so we can extract coefficients for the
  // equation:  y = p1 x + p0
  if (x1 == x2) {
    // Problem computing the polynomial coefficients.
    return;
  }
  double p1 = (y2 - y1) / static_cast<double>(x2 - x1);
  double p0 = y1 - p1 * x1;

  hocr_str << "; baseline " << round(p1 * 1000.0) / 1000.0 << " " << round(p0 * 1000.0) / 1000.0;
}
void AddBoxTohOCR(const tesseract::ResultIterator *it, tesseract::PageIteratorLevel level,std::stringstream &hocr_str) {
  int left, top, right, bottom;
  it->BoundingBox(level, &left, &top, &right, &bottom);
  // This is the only place we use double quotes instead of single quotes,
  // but it may too late to change for consistency
  hocr_str << " title=\"bbox " << left << " " << top << " " << right << " " << bottom;
  // Add baseline coordinates & heights for textlines only.
  if (level == tesseract::RIL_TEXTLINE) {
    AddBaselineCoordsTohOCR(it, level, hocr_str);
    // add custom height measures
    float row_height, descenders, ascenders; // row attributes
    it->RowAttributes(&row_height, &descenders, &ascenders);
    // TODO(rays): Do we want to limit these to a single decimal place?
    hocr_str << "; x_size " << row_height << "; x_descenders " << -descenders << "; x_ascenders "
             << ascenders;
  }
  hocr_str << "\">";
}
std::string nameOfType(int type){
    switch (type)
    {
    case 0:
      return "unkown"; 
      break;
    case 1:
      return "flowing_text";
      break;
    case 2:
      return "heading_text";
      break;
    case 3:
      return "pullout_text";
      break;
    case 4:
      return "equation";
      break;
    case 5:
      return "inline_equation";
      break;
    case 6:
      return "table";
      break;
    case 7:
      return "vertical_text";
      break;
    case 8:
      return "caption_text";
      break;
    case 9:
      return "flowing_image";
      break;
    case 10:
      return "heading_image";
      break;
    case 11:
      return "pullout_image";
      break;
    case 12:
      return "horizontal_line";
      break;
    case 13:
      return "vertical_line";
      break;
    case 14:
      return "noise";
      break;
    case 15:
      return "count";
      break;
    default:
      break;
    }
}

//Compilação
//(Opção 1 | Instalação padrão do tesseract) sudo g++ -o program main.cpp -llept -ltesseract
//(Opção 2 | Tesseract compilado) sudo g++ -o program main.cpp -I/usr/local/include/tesseract -L/usr/local/lib -llept -ltesseract


//Execução: sudo ./program path/to/input/filename path/to/output/filename legacy 
//(OBS: arquivo de entrada deve ser ./png e o arquivo de saída não deve conter a extensão)
int main(int argc, char* argv[])
{
  if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " PATH/TO/INPUT/FILENAME(no extension)" << " /PATH/TO/OUTPUT/FILENAME(no extension)" << " MODE(LSTM or Legacy)" <<std::endl;
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
     if (api->Init(NULL, "por+eng", tesseract::OcrEngineMode::OEM_TESSERACT_ONLY)) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
   }else{
     if (api->Init(NULL, "por+eng", tesseract::OcrEngineMode::OEM_LSTM_ONLY)) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
   }
    api->SetVariable("user_defined_dpi", "300");
    api->SetVariable("textord_tablefind_recognize_tables", "true");
    api->SetVariable("hocr_font_info", "true");
    api->SetVariable("hocr_char_boxes", "true");
    api->SetVariable("lstm_choice_mode", "2");
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
    api->SetPageSegMode(tesseract::PSM_AUTO);
    api->SetImage(image);
    char* outFull = api->GetUTF8Text();

    std::stringstream hocr_str;
    // Use "C" locale (needed for double values x_size and x_descenders).
    hocr_str.imbue(std::locale::classic());
    // Use 8 digits for double values.
    hocr_str.precision(8);
    hocr_str << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" <<
        "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" " <<
        "lang=\"en\">\n <head>\n  <title>";
    hocr_str << "</title>\n" <<
      "  <meta http-equiv=\"Content-Type\" content=\"text/html;" <<
      "charset=utf-8\"/>\n" <<
      "  <meta name='ocr-system' content='tesseract " TESSERACT_VERSION_STR <<
      "' />\n" <<
      "  <meta name='ocr-capabilities' content='ocr_page ocr_carea ocr_par" <<
      " ocr_line ocrx_word ocrp_wconf ocrp_lang ocrp_dir ocrp_font ocrp_fsize";
    hocr_str << "'/>\n" <<
      " </head>\n" <<
      " <body>\n";
    hocr_str << "  <div class='ocr_page'";
    hocr_str << " id='"
            << "page_" << page_id << "'";
    hocr_str << " title='image \"" + std::string(argv[1]) + ".png" + "\"'>\n";
        
    std::unique_ptr<tesseract::ResultIterator> ri(api->GetIterator());
    if(ri != 0){
    while (!ri->Empty(tesseract::RIL_BLOCK)) {
    if (ri->Empty(tesseract::RIL_WORD)) {
      ri->Next(tesseract::RIL_WORD);
      continue;
    }

    // Open any new block/paragraph/textline.
    if (ri->IsAtBeginningOf(tesseract::RIL_BLOCK)) {
      para_is_ltr = true; // reset to default direction
      hocr_str << "   <div class='ocr_carea'"
               << " id='"
               << "block_" << page_id << "_" << bcnt << "'";
      hocr_str << " type='" << nameOfType(ri->BlockType()) << "' conf='" << static_cast<int>(ri->Confidence(tesseract::RIL_BLOCK)) << "'";
      AddBoxTohOCR(ri.get(), tesseract::RIL_BLOCK, hocr_str);
    }
    if (ri->IsAtBeginningOf(tesseract::RIL_PARA)) {
      hocr_str << "\n    <p class='ocr_par'";
      para_is_ltr = ri->ParagraphIsLtr();
      if (!para_is_ltr) {
        hocr_str << " dir='rtl'";
      }
      hocr_str << " id='"
               << "par_" << page_id << "_" << pcnt << "'";
      paragraph_lang = ri->WordRecognitionLanguage();
      if (paragraph_lang) {
        hocr_str << " lang='" << paragraph_lang << "'";
      }
      hocr_str << " type='" << nameOfType(ri->BlockType()) << "' conf='" << static_cast<int>(ri->Confidence(tesseract::RIL_PARA)) << "'";
      AddBoxTohOCR(ri.get(), tesseract::RIL_PARA, hocr_str);
    }
    if (ri->IsAtBeginningOf(tesseract::RIL_TEXTLINE)) {
      hocr_str << "\n     <span class='";
      switch (ri->BlockType()) {
        default:
          hocr_str << "ocr_line";
      }
      hocr_str << "' id='"
               << "line_" << page_id << "_" << lcnt << "'";
      hocr_str << " type='" << nameOfType(ri->BlockType()) << "' conf='" << static_cast<int>(ri->Confidence(tesseract::RIL_TEXTLINE)) << "'";
      AddBoxTohOCR(ri.get(), tesseract::RIL_TEXTLINE, hocr_str);
    }

    // Now, process the word...
    int32_t lstm_choice_mode;
    api->GetIntVariable("lstm_choice_mode", &lstm_choice_mode);
    std::vector<std::vector<std::vector<std::pair<const char *, float>>>> *rawTimestepMap = nullptr;
    std::vector<std::vector<std::pair<const char *, float>>> *CTCMap = nullptr;
    if (lstm_choice_mode) {
      CTCMap = ri->GetBestLSTMSymbolChoices();
      rawTimestepMap = ri->GetRawLSTMTimesteps();
    }
    hocr_str << "\n      <span class='ocrx_word'"
             << " id='"
             << "word_" << page_id << "_" << wcnt << "'";
    int left, top, right, bottom;
    bool bold, italic, underlined, monospace, serif, smallcaps;
    int pointsize, font_id;
    const char *font_name;
    ri->BoundingBox(tesseract::RIL_WORD, &left, &top, &right, &bottom);
    font_name = ri->WordFontAttributes(&bold, &italic, &underlined, &monospace, &serif,
                                           &smallcaps, &pointsize, &font_id);
    hocr_str << " title='bbox " << left << " " << top << " " << right << " " << bottom
             << "; x_wconf " << static_cast<int>(ri->Confidence(tesseract::RIL_WORD));

    if (font_info) {
      if (font_name) {
        hocr_str << "; x_font " << HOcrEscape(font_name).c_str();
      }
      hocr_str << "; x_fsize " << pointsize;
    }
    hocr_str << "'";
    hocr_str << " type='" << nameOfType(ri->BlockType()) << "'";
    const std::unique_ptr<const char[]> wordpheme(ri->GetUTF8Text(tesseract::RIL_WORD));
    const char *lang = ri->WordRecognitionLanguage();
    if (lang && (!paragraph_lang || strcmp(lang, paragraph_lang))) {
      hocr_str << " lang='" << lang << "'";
    }
    hocr_str << " full_text='" << HOcrEscape(wordpheme.get()).c_str() << "'";
    switch (ri->WordDirection()) {
      // Only emit direction if different from current paragraph direction
      case tesseract::DIR_LEFT_TO_RIGHT:
        if (!para_is_ltr)
          hocr_str << " dir='ltr'";
        break;
      case tesseract::DIR_RIGHT_TO_LEFT:
        if (para_is_ltr)
          hocr_str << " dir='rtl'";
        break;
      case tesseract::DIR_MIX:
      case tesseract::DIR_NEUTRAL:
      default: // Do nothing.
        break;
    }
    hocr_str << ">";
    bool last_word_in_line = ri->IsAtFinalElement(tesseract::RIL_TEXTLINE, tesseract::RIL_WORD);
    bool last_word_in_para = ri->IsAtFinalElement(tesseract::RIL_PARA, tesseract::RIL_WORD);
    bool last_word_in_block = ri->IsAtFinalElement(tesseract::RIL_BLOCK, tesseract::RIL_WORD);
    do {
      const std::unique_ptr<const char[]> grapheme(ri->GetUTF8Text(tesseract::RIL_SYMBOL));
      if (grapheme && grapheme[0] != 0) {
        if (hocr_boxes) {
          ri->BoundingBox(tesseract::RIL_SYMBOL, &left, &top, &right, &bottom);
          hocr_str << "\n       <span class='ocrx_symbol' title='x_bboxes " << left << " " << top
                   << " " << right << " " << bottom << "; x_conf " << ri->Confidence(tesseract::RIL_SYMBOL)
                   << "'>";
        }
        hocr_str << HOcrEscape(grapheme.get()).c_str();
        if (hocr_boxes) {
          hocr_str << "</span>";
          tesseract::ChoiceIterator ci(*ri);
          if (ci.Timesteps() != nullptr) {
            std::vector<std::vector<std::pair<const char *, float>>> *symbol = ci.Timesteps();
            hocr_str << "\n        <span class='ocr_timesteps'"
                     << " id='"
                     << "symbol_" << page_id << "_" << wcnt << "_" << scnt << "'>";
            for (auto timestep : *symbol) {
              hocr_str << "\n         <span class='ocrx_timestep'"
                       << " id='"
                       << "timestep" << page_id << "_" << wcnt << "_" << tcnt << "'>";
              for (auto conf : timestep) {
                hocr_str << "\n          <span class='ocrx_timestep_choice'"
                         << " id='"
                         << "choice_" << page_id << "_" << wcnt << "_" << ccnt << "'"
                         << " title='x_confs " << int(conf.second * 100) << "'>"
                         << HOcrEscape(conf.first).c_str() << "</span>";
                ++ccnt;
              }
              hocr_str << "</span>";
              ++tcnt;
            }
            hocr_str << "\n        </span>";
            ++scnt;
          } 
          if (lstm_choice_mode == 2) {
            tesseract::ChoiceIterator ci(*ri);
            hocr_str << "\n        <span class='ocrx_choices'"
                     << " id='"
                     << "lstm_choices_" << page_id << "_" << wcnt << "_" << tcnt << "'>";
            do {
              const char *choice = ci.GetUTF8Text();
              float choiceconf = ci.Confidence();
              if (choice != nullptr) {
                hocr_str << "\n         <span class='ocrx_choice'"
                         << " id='"
                         << "choice_" << page_id << "_" << wcnt << "_" << ccnt << "'"
                         << " title='x_confs " << choiceconf << "'>" << HOcrEscape(choice).c_str()
                         << "</span>";
                ccnt++;
              }
            } while (ci.Next());
            hocr_str << "\n        </span>";
            tcnt++;
          }
        }
      }
      ri->Next(tesseract::RIL_SYMBOL);
    } while (!ri->Empty(tesseract::RIL_BLOCK) && !ri->IsAtBeginningOf(tesseract::RIL_WORD));
    // If the lstm choice mode is required it is added here
    if (lstm_choice_mode == 1 && !hocr_boxes && rawTimestepMap != nullptr) {
      for (auto symbol : *rawTimestepMap) {
        hocr_str << "\n       <span class='ocr_symbol'"
                 << " id='"
                 << "symbol_" << page_id << "_" << wcnt << "_" << scnt << "'>";
        for (auto timestep : symbol) {
          hocr_str << "\n        <span class='ocrx_cinfo'"
                   << " id='"
                   << "timestep" << page_id << "_" << wcnt << "_" << tcnt << "'>";
          for (auto conf : timestep) {
            hocr_str << "\n         <span class='ocrx_cinfo'"
                     << " id='"
                     << "choice_" << page_id << "_" << wcnt << "_" << ccnt << "'"
                     << " title='x_confs " << int(conf.second * 100) << "'>"
                     << HOcrEscape(conf.first).c_str() << "</span>";
            ++ccnt;
          }
          hocr_str << "</span>";
          ++tcnt;
        }
        hocr_str << "</span>";
        ++scnt;
      }
    } else if (lstm_choice_mode == 2 && !hocr_boxes && CTCMap != nullptr) {
      for (auto timestep : *CTCMap) {
        if (timestep.size() > 0) {
          hocr_str << "\n       <span class='ocrx_choices'"
                   << " id='"
                   << "lstm_choices_" << page_id << "_" << wcnt << "_" << tcnt << "'>";
          for (auto &j : timestep) {
            float conf = 100 - 5 * j.second;
            if (conf < 0.0f)
              conf = 0.0f;
            if (conf > 100.0f)
              conf = 100.0f;
            hocr_str << "\n        <span class='ocrx_choice'"
                     << " id='"
                     << "choice_" << page_id << "_" << wcnt << "_" << ccnt << "'"
                     << " title='x_confs " << conf << "'>" << HOcrEscape(j.first).c_str()
                     << "</span>";
            ccnt++;
          }
          hocr_str << "</span>";
          tcnt++;
        }
      }
    }
    // Close ocrx_word.
    if (hocr_boxes || lstm_choice_mode > 0) {
      hocr_str << "\n      ";
    }
    hocr_str << "</span>";
    tcnt = 1;
    ccnt = 1;
    wcnt++;
    // Close any ending block/paragraph/textline.
    if (last_word_in_line) {
      hocr_str << "\n     </span>";
      lcnt++;
    }
    if (last_word_in_para) {
      hocr_str << "\n    </p>\n";
      pcnt++;
      para_is_ltr = true; // back to default direction
    }
    if (last_word_in_block) {
      hocr_str << "   </div>\n";
      bcnt++;
    }
  }
    }
    hocr_str << "  </div>\n";
    hocr_str << " </body>\n</html>\n";

    const std::string &text = hocr_str.str();
    
    outFile = std::string(argv[2]);
    ofstream myfile;
    myfile.open(outFile);
    myfile << hocr_str.str();
    myfile.close();
    
    // Destroy used object and release memory
    api->End();
    delete api;
    delete [] outFull;
    pixDestroy(&image);

    return 0;
}