#include<iostream>
#include<cstdlib>
#include<string>
#include<fstream>
#include<sys/stat.h>
#include<Windows.h>

using namespace std;



const string REGEN_FILEPATH = "./regen_legends.edt";
const string CSV_FILEPATH = "./sheet.csv";

// 格式空行
const string csv_headline = "\xef\xbb\xbf"
"\"DETAILED_FUTURE_REGEN\",\"First Name\",\"Common Name\",\"Last Name\",\"Birth date\","
"\"Nation\",\"Favourite Team\",\"Ethnicity\",\"Skin Tone\","
"\"Hair Colour\",\"Height(in cm)\",\"Weight(in kg)\",\"Preferred Foot\",\"Preferred Position\","
"\"Favourite Number\",\"Birth City\",\"CA\",\"PA\",\"Club ID\""
"\n";


//这是句段模板:
//"DETAILED_FUTURE_REGEN" "First Name" "Common Name" "Last Name" "Birth date(dd/mm/yyyy)" 
// "Nation (first nationality)" "Favourite Team" "Ethnicity" "Skin Tone"
// "Hair Colour" "Height (in cm)" "Weight (in kg)" "Preferred Foot" "Preferred Position" 
// "Favourite Number" "Birth City""CA""PA""Club ID"
//参考范例:
//"DETAILED_FUTURE_REGEN" "Zbigniew" "" "Boniek" "03/03/2006" 
// "787" "1468" "0" "2" "2" "181" "76" "2" "ATTACKING_MIDFIELDER_CENTRAL" 
// "11" "Bydgoszcz" "105" "170" "1468"

// 这是部分编码形式填写的参数
// Ethnicity（种族）:
// Unknown/random = -1
// Northern europe = 0
// Mediterrean/hispanic = 1
// North african/middle eastern = 2,
// African/caribbean = 3,
// Asian = 4,
// South east asian = 5,
// Pacific islander = 6,
// Native american = 7,
// Native australian = 8,
// Mixed race white/black = 9
// East asian = 10

// Skin tone（肤色）:
// a value of 1-20, with 1 being the lightest and 20 being the darkest

// Hair colour（发色）:
// Unknown/random = 0,
// Blond = 1,
// Light brown = 2,
// Dark brown = 3,
// Red = 4,
// Black = 5,
// Grey = 6,
// Bald = 7

// Preferred foot（惯用脚）:
// Right only = 0,
// Left only = 1,
// Right preferred = 2,
// Left preferred = 3,
// Both = 4

// Positions（位置）:
// GOALKEEPER, DEFENDER_LEFT_SIDE, DEFENDER_RIGHT_SIDE, DEFENDER_CENTRAL, MIDFIELDER_LEFT_SIDE, MIDFIELDER_RIGHT_SIDE, MIDFIELDER_CENTRAL, ATTACKING_MIDFIELDER_LEFT_SIDE, ATTACKING_MIDFIELDER_RIGHT_SIDE, ATTACKING_MIDFIELDER_CENTRAL, ATTACKER_CENTRAL






// while check if the filepath existed
bool file_existed(const string& filepath)
{
  struct stat info;
  if (stat(filepath.c_str(), &info) != 0) {
    if (errno == ENOENT) {
      // 文件不存在
      return false;
    }
    // error here
    perror("Error while check file status");
    return false;
  }
  return true;
}





int main()
{
  if (!file_existed(REGEN_FILEPATH)) {
    cout << "file not existed\n";
    return  -1;
  }

  // Window-terminal设置UTF8
  system("chcp 65001");
  system("cls");

  ifstream ifs(REGEN_FILEPATH, ios::ate);
  if (!ifs.is_open()) {
    cout << "REGEN_FILE open failed\n";
    return -1;
  }
  size_t reg_filesize = ifs.tellg();
  cout << "FILESIZE:" << reg_filesize << std::endl;
  ifs.seekg(0);

  std::ofstream ofs(CSV_FILEPATH, std::ios::trunc);
  if (!ofs.is_open()) {
    cout << "CSV_FILE open failed\n";
    return -1;
  }
  // 写入BOM（Byte Order Mark）
  // UTF-8
  ofs << csv_headline;

  std::string line;
  size_t while_depth = 0;
  while (std::getline(ifs, line)) {
    size_t finder=0;
    while ((finder=line.find("\" \"",finder)) != string::npos) {
      line[finder + 1] = ',';
    }
    ofs << line << "\n";
    //cout << while_depth++ << ": " << line << endl;
    if (while_depth == -1)
      break;
  }
  ifs.close();
  ofs.close();

  return 0;
}


