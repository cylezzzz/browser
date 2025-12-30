\
/* src/core/extract/HtmlExtractor.cpp */
#include "core/extract/HtmlExtractor.h"
#include "util/Log.h"
#include <gumbo.h>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace core::extract {

static void appendText(std::string& out, const std::string& t) {
  if (t.empty()) return;
  if (!out.empty() && out.back() != '\n') out.push_back('\n');
  out += t;
}

static bool isVisibleTextNode(const GumboNode* node) {
  if (!node || node->type != GUMBO_NODE_TEXT) return false;
  return true;
}

static bool isSkippableTag(GumboTag tag) {
  switch (tag) {
    case GUMBO_TAG_SCRIPT:
    case GUMBO_TAG_STYLE:
    case GUMBO_TAG_NOSCRIPT:
    case GUMBO_TAG_NAV:
    case GUMBO_TAG_FOOTER:
    case GUMBO_TAG_HEADER:
    case GUMBO_TAG_ASIDE:
      return true;
    default:
      return false;
  }
}

static std::string getAttribute(const GumboElement& el, const char* name) {
  GumboAttribute* a = gumbo_get_attribute(&el.attributes, name);
  if (!a || !a->value) return "";
  return a->value;
}

static void walk(const GumboNode* node,
                 std::vector<std::string>& headings,
                 std::vector<std::string>& links,
                 std::vector<std::string>& paraTexts,
                 bool inSkippable) {
  if (!node) return;

  if (node->type == GUMBO_NODE_ELEMENT) {
    GumboTag tag = node->v.element.tag;
    bool skip = inSkippable || isSkippableTag(tag);

    if (!skip) {
      if (tag == GUMBO_TAG_A) {
        auto href = getAttribute(node->v.element, "href");
        if (!href.empty()) links.push_back(href);
      }
    }

    if (!skip && (tag == GUMBO_TAG_H1 || tag == GUMBO_TAG_H2 || tag == GUMBO_TAG_H3)) {
      // collect text under this node
      std::string txt;
      // naive: recurse and gather text
      GumboVector* children = &node->v.element.children;
      for (unsigned i = 0; i < children->length; ++i) {
        walk(static_cast<GumboNode*>(children->data[i]), headings, links, paraTexts, skip);
      }
      // headings are collected by parsing after; simpler: ignore here
    }

    if (!skip && (tag == GUMBO_TAG_P || tag == GUMBO_TAG_LI || tag == GUMBO_TAG_ARTICLE || tag == GUMBO_TAG_MAIN)) {
      // extract visible text from subtree as paragraph-ish
      std::string txt;
      std::vector<const GumboNode*> stack;
      stack.push_back(node);
      while (!stack.empty()) {
        const GumboNode* n = stack.back();
        stack.pop_back();
        if (n->type == GUMBO_NODE_TEXT && n->v.text.text) {
          txt += n->v.text.text;
          txt.push_back(' ');
        } else if (n->type == GUMBO_NODE_ELEMENT) {
          GumboTag t = n->v.element.tag;
          if (isSkippableTag(t)) continue;
          GumboVector* ch = &n->v.element.children;
          for (unsigned i = 0; i < ch->length; ++i) {
            stack.push_back(static_cast<GumboNode*>(ch->data[i]));
          }
        }
      }
      // normalize whitespace
      txt.erase(std::unique(txt.begin(), txt.end(), [](char a, char b){
        return std::isspace((unsigned char)a) && std::isspace((unsigned char)b);
      }), txt.end());
      if (txt.size() > 40) paraTexts.push_back(txt);
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned i = 0; i < children->length; ++i) {
      walk(static_cast<GumboNode*>(children->data[i]), headings, links, paraTexts, skip);
    }
  } else if (isVisibleTextNode(node)) {
    // no-op; handled in paragraph scanning
  }
}

static void findMeta(const GumboNode* node, std::string& title, std::string& desc, std::string& canonical) {
  if (!node) return;
  if (node->type == GUMBO_NODE_ELEMENT) {
    GumboTag tag = node->v.element.tag;
    if (tag == GUMBO_TAG_TITLE) {
      GumboVector* ch = &node->v.element.children;
      for (unsigned i = 0; i < ch->length; ++i) {
        GumboNode* c = static_cast<GumboNode*>(ch->data[i]);
        if (c->type == GUMBO_NODE_TEXT && c->v.text.text) {
          title = c->v.text.text;
          break;
        }
      }
    }
    if (tag == GUMBO_TAG_META) {
      auto name = getAttribute(node->v.element, "name");
      auto prop = getAttribute(node->v.element, "property");
      auto content = getAttribute(node->v.element, "content");
      if (!content.empty()) {
        if (name == "description" && desc.empty()) desc = content;
        if (prop == "og:description" && desc.empty()) desc = content;
      }
    }
    if (tag == GUMBO_TAG_LINK) {
      auto rel = getAttribute(node->v.element, "rel");
      auto href = getAttribute(node->v.element, "href");
      if (rel == "canonical" && !href.empty()) canonical = href;
    }
    GumboVector* children = &node->v.element.children;
    for (unsigned i = 0; i < children->length; ++i) {
      findMeta(static_cast<GumboNode*>(children->data[i]), title, desc, canonical);
    }
  }
}

std::string HtmlExtractor::stripWhitespace(const std::string& s) {
  std::string out = s;
  out.erase(out.begin(), std::find_if(out.begin(), out.end(), [](unsigned char ch){ return !std::isspace(ch); }));
  out.erase(std::find_if(out.rbegin(), out.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), out.end());
  return out;
}

ExtractedPage HtmlExtractor::extract(const std::string& html, const std::string& baseUrl) {
  ExtractedPage ep;
  GumboOutput* output = gumbo_parse(html.c_str());
  if (!output) return ep;

  findMeta(output->root, ep.title, ep.description, ep.canonicalUrl);

  std::vector<std::string> paraTexts;
  walk(output->root, ep.headings, ep.links, paraTexts, false);

  // Simple "main content" heuristic: take top paragraphs by length
  std::sort(paraTexts.begin(), paraTexts.end(), [](const std::string& a, const std::string& b){
    return a.size() > b.size();
  });

  const int maxBlocks = 24;
  int take = std::min<int>((int)paraTexts.size(), maxBlocks);
  ep.blocks.reserve(take);
  std::ostringstream full;
  for (int i = 0; i < take; ++i) {
    Block b;
    b.id = "block_" + std::string(i < 9 ? "00" : (i < 99 ? "0" : "")) + std::to_string(i + 1);
    b.text = stripWhitespace(paraTexts[i]);
    ep.blocks.push_back(b);
    full << "[" << b.id << "] " << b.text << "\n\n";
  }
  ep.fullText = full.str();
  ep.canonicalUrl = ep.canonicalUrl.empty() ? baseUrl : ep.canonicalUrl;

  gumbo_destroy_output(&kGumboDefaultOptions, output);
  return ep;
}

} // namespace core::extract
