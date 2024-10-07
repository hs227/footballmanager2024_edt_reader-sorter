#include<iostream>
#include<cstdlib>
#include<string>
#include<fstream>
#include<sys/stat.h>
#include<cassert>
#include<set>
#include<filesystem>
#include<Windows.h>

using namespace std;

namespace fs = std::filesystem;

const string REGEN_FILEPATH = "./regen_legends.edt";
const string CSV_FILEPATH = "./sheet.csv";
const string SORTER_TMP_DIR = "./sort_tmp/";

// max_line_size in tmp_bock
// 决定了每次合并块的大小（行数）
const size_t BLOCK_LINE_SIZE = 1024;
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



// will delete all files in the dir recursively
void empty_dir(const string& filepath) 
{
  for (auto& entry : std::filesystem::recursive_directory_iterator(filepath)) {
    if (entry.is_directory()) {
      empty_dir(entry.path().string());
    }else {
      fs::remove(entry);
    }
  }
}

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

// for tmp_block write
void block_writer(const string& line, const size_t line_num, const size_t block_num, const size_t layer_num)
{
  static ofstream bw_ofs;
  if (line_num == 0 && bw_ofs.is_open()) {
    // 需要切换文件了
    bw_ofs.close();
    if (line == "END") {
      // just close the file rather than replace
      return;
    }
  }
  if (!bw_ofs.is_open()) {
    //打开新文件
    string block_filename = SORTER_TMP_DIR + "L" + std::to_string(layer_num) + "B" + std::to_string(block_num) + ".csv";
    bw_ofs.open(block_filename, ios::trunc);
    if (!bw_ofs.is_open()) {
      cout << block_filename << " open failed\n";
      exit(EXIT_FAILURE);
    }
    //为新文件设置UTF-8
    // 写入BOM（Byte Order Mark）
    // UTF-8
    bw_ofs << csv_headline;
  }

  bw_ofs << line << endl;
}

// for tmp_block copy
// for merge
void block_copyer(const string& from_block_path, const string& to_block_path)
{
  size_t file_size;
  size_t block_size = 1024;
  char* translate_zone = new char[block_size];
  // init
  ifstream from_ifs(from_block_path, ios::binary | ios::ate);
  if (!from_ifs.is_open()) {
    cout << "block_copyer::from::" << from_block_path << " open failed\n";
    exit(EXIT_FAILURE);
  }
  file_size = from_ifs.tellg();
  from_ifs.seekg(0);
  ofstream to_ofs(to_block_path, ios::binary | ios::trunc);
  if (!to_ofs.is_open()) {
    cout << "block_copyer::to::" << to_block_path << " open failed\n";
    exit(EXIT_FAILURE);
  }
  // real copy
  while (file_size != 0) {
    if (block_size > file_size)
      block_size = file_size;
    from_ifs.read(translate_zone, block_size);
    to_ofs.write(translate_zone, block_size);
    file_size -= block_size;
  }
  // finish
  from_ifs.close();
  to_ofs.close();
  delete[] translate_zone;
}

void block_copyer(const size_t from_layer, const size_t from_block, const size_t to_layer, const size_t to_block) 
{
  const string from_block_path= SORTER_TMP_DIR + "L" + to_string(from_layer) + "B" + to_string(from_block) + ".csv";
  const string to_block_path = SORTER_TMP_DIR + "L" + to_string(to_layer) + "B" + to_string(to_block) + ".csv";
  
  block_copyer(from_block_path, to_block_path);
}




// input: line
// output: birthday(in)
void get_line_birthday(const string& line, string& birthday)
{
  size_t left = -1, right;
  for (int i = 0; i < 4; ++i) {
    left = line.find(',', left + 1);
    if (left == string::npos)
      break;
  }
  if (left == string::npos) {
    birthday = "";
    return;
  }
  left += 2;
  right = line.find('"', left);
  birthday = line.substr(left, right - left);
}

// in: birthday
// out:year(in),month(in),day(in)
void split_birthday(const string& birthday, int& year, int& month, int& day) 
{
  size_t first_slash, second_slash;
  first_slash = birthday.find('/');
  second_slash = birthday.find('/', first_slash + 1);
  if (first_slash == string::npos ||
    second_slash == string::npos ||
    first_slash == second_slash) {
    
    if(birthday!="")
      year = month = day = 0;
    return;
  }
  day = stoi(birthday.substr(0, first_slash));
  month = stoi(birthday.substr(first_slash + 1, second_slash - first_slash-1));
  year = stoi(birthday.substr(second_slash + 1));
}

// in: str_birthday
// out: int_birthday(in)
void split_birthday_then_merge(const string& str_birthday,int& int_birthday) 
{
  int year, month, day;
  split_birthday(str_birthday, year, month, day);
  int_birthday = year;
  int_birthday = int_birthday * 100 + month;
  int_birthday = int_birthday * 100 + day;
}

class compared_line
{
public:
  std::string line;
  //int year, month, day;
  int birthday;

  void init() 
  {
    std::string str_birthday;
    get_line_birthday(line, str_birthday);
    split_birthday_then_merge(str_birthday, birthday);
  }

  compared_line() 
    {};
  compared_line(const string& line_) 
    :line(line_)
  {
    init();
  }
  //bool operator<(const compared_line& rhs) const
  //{
  //  if (year < rhs.year)
  //    return true;
  //  if (year == rhs.year && month < rhs.month)
  //    return true;
  //  if (year == rhs.year && month == rhs.month && day < rhs.day)
  //    return true;
  //  return false;
  //}
 
  bool operator<(const compared_line& rhs) const
  {
    return birthday < rhs.birthday;
  }


};

// make the assigned (by layer and block index) block sorted
void sort_one_block(const size_t layer_idx, const size_t b_idx)
{
  string block_path = SORTER_TMP_DIR + "L" + to_string(layer_idx) + "B" + to_string(b_idx) + ".csv";
  string blockn_path = SORTER_TMP_DIR + "L" + to_string(layer_idx+1) + "B" + to_string(b_idx) + ".csv";
  ifstream block_ifs(block_path);
  if (!block_ifs.is_open()) {
    cout << block_path << " ifs: open failed\n";
    exit(EXIT_FAILURE);
  }

  multiset<compared_line> line_set;
  string line;
  // 空行
  getline(block_ifs, line);
  //数据行
  while (getline(block_ifs, line)) {
    compared_line new_line(line);
    int size = line_set.size();
    line_set.insert(new_line);
    if (size == line_set.size()) {
      cout << "!!!" << line_set.size() << " " << line << endl;
    }
  }
  block_ifs.close();

  ofstream block_ofs(blockn_path, ios::trunc);
  if (!block_ofs.is_open()) {
    cout << block_path << " ofs: open failed\n";
    exit(EXIT_FAILURE);
  }
  // 写入BOM（Byte Order Mark）
  // UTF-8
  block_ofs << csv_headline;
  int entry_num = 0;
  for (const auto& entry : line_set) {
    block_ofs << entry.line << "\n";
    entry_num++;
  }
  if (entry_num != line_set.size()) {
    cout << "!!! " << entry_num << " : " << line_set.size()<<"\n";
  }
  block_ofs.close();
}


bool two_block_merge_sorted(const size_t layer_idx, const size_t b1_idx, const size_t b2_idx, const size_t bn_idx)
{
  string block1_path = SORTER_TMP_DIR + "L" + to_string(layer_idx) + "B" + to_string(b1_idx) + ".csv";
  string block2_path = SORTER_TMP_DIR + "L" + to_string(layer_idx) + "B" + to_string(b2_idx) + ".csv";
  string blockn_path = SORTER_TMP_DIR + "L" + to_string(layer_idx + 1) + "B" + to_string(bn_idx) + ".csv";

  ifstream b1_input(block1_path);
  if (!b1_input.is_open()) {
    cout << block1_path + ": open failed\n";
    return false;
  }
  ifstream b2_input(block2_path);
  if (!b2_input.is_open()) {
    b1_input.close();
    cout << block2_path + ": open failed\n";
    return false;
  }
  ofstream bn_output(blockn_path);
  if (!bn_output.is_open()) {
    b1_input.close();
    b2_input.close();
    cout << blockn_path + " : open failed\n";
    return false;
  }
  // 写入BOM（Byte Order Mark）
  // UTF-8
  bn_output << csv_headline;


  compared_line cline1;
  compared_line cline2;

  //空行
  getline(b1_input, cline1.line);
  getline(b2_input, cline2.line);
  cline1.line = "";
  cline2.line = "";
  //数据行
  for(;;) {
    if (cline1.line == "") {
      getline(b1_input, cline1.line);
      cline1.init();
    }
    if (cline2.line == "") {
      getline(b2_input, cline2.line);
      cline2.init();
    }

    if (cline1.line == "" && cline2.line == "")
      break;


    if (cline1.line == "") {
      bn_output << cline2.line << "\n";
      cline2.line = "";
    }
    else if (cline2.line == "") {
      bn_output << cline1.line << "\n";
      cline1.line = "";
    }
    else {
      if (cline1 < cline2) {
        bn_output << cline1.line << "\n";
        cline1.line = "";
      }
      else {      
        bn_output << cline2.line << "\n";
        cline2.line = "";
      }

    }

  }

  bn_output.close();

}


int main()
{
  if (!file_existed(CSV_FILEPATH)) {
    cout << "file not existed\n";
    return  -1;
  }

  // Window-terminal设置UTF8
  system("chcp 65001");
  system("cls");
  

  // clean the tmp
  empty_dir(SORTER_TMP_DIR);

  ifstream ifs(CSV_FILEPATH);
  if (!ifs.is_open()) {
    cout << "ifs open failed\n";
    return -1;
  }

  // 1.分块

  string line;
  size_t while_depth = 0;
  size_t block_num = 0;
  size_t line_num = 0;
  size_t layer_num = 0;

  // 空行
  getline(ifs, line);
  //数据行
  while (getline(ifs, line)) {
    //cout<<block_num+" "<<line_num<<": "<<line<<std::endl;
    block_writer(line, line_num, block_num, layer_num);
    line_num++;
    while_depth++;
    if (line_num == BLOCK_LINE_SIZE) {
      block_num++;
      line_num = 0;
    }
  }
  block_writer("END", 0, 0, 0);// close out file
  if(line_num!=0)
    block_num++;
  ifs.close();
  cout << "all records: " << while_depth << endl;
  cout << "all blocks: "<<block_num << endl;

  // 2.开始排序，大块排序
  // 2.1每块完成自身排序
  for (size_t i = 0; i < block_num; ++i) {
    sort_one_block(layer_num, i);
  }
  int each_layer_stride = block_num / 2;
  int each_layer_block_num = block_num;

  // 2.2合并排序
  for (; each_layer_block_num > 1;each_layer_stride=each_layer_block_num/2) {
    layer_num++;
    int next_layer_block_num = 0;
    for (int i = 0; i < each_layer_stride; ++i) {
      two_block_merge_sorted(layer_num, i, i + each_layer_stride, i);
      next_layer_block_num++;
    }
    if (each_layer_block_num % 2 != 0) {
      // 奇数块合并时的多余块轮空
      block_copyer(layer_num, each_layer_block_num - 1, layer_num + 1, next_layer_block_num);
      next_layer_block_num++;
    }
    each_layer_block_num = next_layer_block_num;
  }

  // 3.将保存结果
  const string from_block_path = SORTER_TMP_DIR + "L" + to_string(layer_num+1) + "B0" + ".csv";
  const string to_block_path = SORTER_TMP_DIR + "result.csv";
  block_copyer(from_block_path, to_block_path);
 

  


  return 0;
}


