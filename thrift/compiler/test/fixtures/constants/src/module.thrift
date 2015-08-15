const i32 myInt = 1337;
const string name = "Mark Zuckerberg";
const list<map<string, i32>> states = [
  {"San Diego": 3211000, "Sacramento": 479600, "SF": 837400},
  {"New York": 8406000, "Albany": 98400}
];
const double x = 1.0;
const double y = 1000000;
const double z = 1000000000.0;

enum City { NYC, MPK, SEA, LON }
enum Company { FACEBOOK, WHATSAPP, OCULUS, INSTAGRAM }

struct Internship {
  1: i32 weeks;
  2: string title;
  3: Company employer;
}

const Internship instagram = {
  "weeks": 12,
  "title": "Software Engineer",
  "employer": Company.INSTAGRAM
};

const list<Internship> internList = [
  instagram,
  {
    "weeks": 10,
    "title": "Sales Intern",
    "employer": Company.FACEBOOK
  }
];

const string apostrophe = "'";
const string tripleApostrophe = "'''";
const string quotationMark = '"'; //" //fix syntax highlighting
const string backslash = "\\";
const string escaped_a = "\x61";

const map<string, i32> char2ascii = {
  "'": 39,
  '"': 34,
  "\\": 92,
  "\x61": 97,
};
