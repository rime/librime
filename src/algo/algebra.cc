//
// Copyleft RIME Developers
// License: GPLv3
//
// 2012-01-19 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <fstream>
#include <boost/foreach.hpp>
#include <rime/algo/algebra.h>
#include <rime/algo/calculus.h>

namespace rime {

bool Script::AddSyllable(const std::string& syllable) {
  if (find(syllable) != end())
    return false;
  Spelling spelling(syllable);
  (*this)[syllable].push_back(spelling);
  return true;
}

void Script::Merge(const std::string& s,
                   const SpellingProperties& sp,
                   const std::vector<Spelling>& v) {
  std::vector<Spelling>& m((*this)[s]);
  BOOST_FOREACH(const Spelling& x, v) {
    Spelling y(x);
    SpellingProperties& yy(y.properties);
    {
      if (sp.type > yy.type)
        yy.type = sp.type;
      yy.credibility *= sp.credibility;
      if (!sp.tips.empty())
        yy.tips = sp.tips;
    }
    std::vector<Spelling>::iterator e = std::find(m.begin(), m.end(), x);
    if (e == m.end()) {
      m.push_back(y);
    }
    else {
      SpellingProperties& zz(e->properties);
      if (yy.type < zz.type)
        zz.type = yy.type;
      if (yy.credibility > zz.credibility)
        zz.credibility = yy.credibility;
      zz.tips.clear();
    }
  }
}

void Script::Dump(const std::string& file_name) const {
  std::ofstream out(file_name.c_str());
  BOOST_FOREACH(const value_type& v, *this) {
    bool first = true;
    BOOST_FOREACH(const Spelling& s, v.second) {
      out << (first ? v.first : "") << '\t'
          << s.str << '\t'
          << "-ac?!"[s.properties.type] << '\t'
          << s.properties.credibility << '\t'
          << s.properties.tips << std::endl;
      first = false;
    }
  }
  out.close();
}

bool Projection::Load(ConfigListPtr settings) {
  if (!settings) return false;
  calculation_.clear();
  Calculus calc;
  bool success = true;
  for (size_t i = 0; i < settings->size(); ++i) {
    ConfigValuePtr v(settings->GetValueAt(i));
    if (!v) {
      LOG(ERROR) << "Error loading formula #" << (i + 1) << ".";
      success = false;
      break;
    }
    const std::string &formula(v->str());
    shared_ptr<Calculation> x;
    try {
      x.reset(calc.Parse(formula));
    }
    catch (boost::regex_error& e) {
      LOG(ERROR) << "Error parsing formula '" << formula << "': " << e.what();
    }
    if (!x) {
      LOG(ERROR) << "Error loading spelling algebra definition #" << (i + 1)
                 << ": '" << formula << "'.";
      success = false;
      break;
    }
    calculation_.push_back(x);
  }
  if (!success) {
    calculation_.clear();
  }
  return success;
}

bool Projection::Apply(std::string* value) {
  if (!value || value->empty())
    return false;
  bool modified = false;
  Spelling s(*value);
  BOOST_FOREACH(shared_ptr<Calculation>& x, calculation_) {
    try {
      if (x->Apply(&s))
        modified = true;
    }
    catch (std::runtime_error& e) {
      LOG(ERROR) << "Error applying calculation: " << e.what();
      return false;
    }
  }
  if (modified)
    value->assign(s.str);
  return modified;
}

bool Projection::Apply(Script* value) {
  if (!value || value->empty())
    return false;
  bool modified = false;
  int round = 0;
  BOOST_FOREACH(shared_ptr<Calculation>& x, calculation_) {
    ++round;
    DLOG(INFO) << "round #" << round;
    Script temp;
    BOOST_FOREACH(const Script::value_type& v, *value) {
      Spelling s(v.first);
      bool applied = false;
      try {
        applied = x->Apply(&s);
      }
      catch (std::runtime_error& e) {
        LOG(ERROR) << "Error applying calculation: " << e.what();
        return false;
      }
      if (applied) {
        modified = true;
        if (!x->deletion()) {
          temp.Merge(v.first, SpellingProperties(), v.second);
        }
        if (x->addition() && !s.str.empty()) {
          temp.Merge(s.str, s.properties, v.second);
        }
      }
      else {
        temp.Merge(v.first, SpellingProperties(), v.second);
      }
    }
    value->swap(temp);
  }
  return modified;
}

}  // namespace rime
