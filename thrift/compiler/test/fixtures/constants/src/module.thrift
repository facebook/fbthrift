const i32 myInt = 1337;
const string name = "Mark Zuckerberg";
const list<map<string, i32>> states = [
  {"San Diego": 3211000, "Sacramento": 479600, "SF": 837400},
  {"New York": 8406000, "Albany": 98400}
];

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
