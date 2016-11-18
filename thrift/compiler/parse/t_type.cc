#include <thrift/compiler/parse/t_type.h>
#include <thrift/compiler/parse/t_program.h>

#include <string>
#include <sstream>
#include <openssl/sha.h>

uint64_t t_type::get_type_id() const {
  union {
    uint64_t val;
    unsigned char buf[SHA_DIGEST_LENGTH];
  } u;
  std::string name = get_full_name();
  SHA1(reinterpret_cast<const unsigned char*>(name.data()), name.size(), u.buf);
  uint64_t hash = folly::Endian::little(u.val);
  TypeValue tv = get_type_value();

  return (hash & ~t_types::kTypeMask) | tv;
}

std::string t_type::make_full_name(const char* prefix) const {
  std::ostringstream os;
  os << prefix << " ";
  if (program_) {
    os << program_->get_name() << ".";
  }
  os << name_;
  return os.str();
}

void override_annotations(std::map<std::string, std::string>& where,
                          const std::map<std::string, std::string>& from) {
  for (std::map<std::string, std::string>::const_iterator it =
       from.begin(); it != from.end(); ++it) {
    where[it->first] = it->second;
  }
}
