// Released under the MIT License. See LICENSE for details.

#include "ballistica/scene/node/text_node.h"

#include <algorithm>

#include "ballistica/generic/utils.h"
#include "ballistica/graphics/component/simple_component.h"
#include "ballistica/graphics/graphics.h"
#include "ballistica/graphics/text/text_graphics.h"
#include "ballistica/python/python.h"
#include "ballistica/scene/node/node_attribute.h"
#include "ballistica/scene/node/node_type.h"
#include "ballistica/scene/scene.h"

namespace ballistica {

class TextNodeType : public NodeType {
 public:
#define BA_NODE_TYPE_CLASS TextNode
  BA_NODE_CREATE_CALL(CreateText);
  BA_FLOAT_ATTR(opacity, opacity, set_opacity);
  BA_FLOAT_ATTR(trail_opacity, trail_opacity, set_trail_opacity);
  BA_FLOAT_ATTR(project_scale, project_scale, set_project_scale);
  BA_FLOAT_ATTR(scale, scale, set_scale);
  BA_FLOAT_ARRAY_ATTR(position, position, SetPosition);
  BA_STRING_ATTR(text, getText, SetText);
  BA_BOOL_ATTR(big, big, SetBig);
  BA_BOOL_ATTR(trail, trail, set_trail);
  BA_FLOAT_ARRAY_ATTR(color, color, SetColor);
  BA_FLOAT_ARRAY_ATTR(trailcolor, trail_color, SetTrailColor);
  BA_FLOAT_ATTR(trail_project_scale, trail_project_scale,
                set_trail_project_scale);
  BA_BOOL_ATTR(opacity_scales_shadow, opacity_scales_shadow,
               set_opacity_scales_shadow);
  BA_STRING_ATTR(h_align, GetHAlign, SetHAlign);
  BA_STRING_ATTR(v_align, GetVAlign, SetVAlign);
  BA_STRING_ATTR(h_attach, GetHAttach, SetHAttach);
  BA_STRING_ATTR(v_attach, GetVAttach, SetVAttach);
  BA_BOOL_ATTR(in_world, in_world, set_in_world);
  BA_FLOAT_ATTR(tilt_translate, tilt_translate, set_tilt_translate);
  BA_FLOAT_ATTR(maxwidth, max_width, set_max_width);
  BA_FLOAT_ATTR(shadow, shadow, set_shadow);
  BA_FLOAT_ATTR(flatness, flatness, set_flatness);
  BA_BOOL_ATTR(client_only, client_only, set_client_only);
  BA_BOOL_ATTR(host_only, host_only, set_host_only);
  BA_FLOAT_ATTR(vr_depth, vr_depth, set_vr_depth);
  BA_FLOAT_ATTR(rotate, rotate, set_rotate);
  BA_BOOL_ATTR(front, front, set_front);
#undef BA_NODE_TYPE_CLASS
  TextNodeType()
      : NodeType("text", CreateText),
        opacity(this),
        trail_opacity(this),
        project_scale(this),
        scale(this),
        position(this),
        text(this),
        big(this),
        trail(this),
        color(this),
        trailcolor(this),
        trail_project_scale(this),
        opacity_scales_shadow(this),
        h_align(this),
        v_align(this),
        h_attach(this),
        v_attach(this),
        in_world(this),
        tilt_translate(this),
        maxwidth(this),
        shadow(this),
        flatness(this),
        client_only(this),
        host_only(this),
        vr_depth(this),
        rotate(this),
        front(this) {}
};
static NodeType* node_type{};

auto TextNode::InitType() -> NodeType* {
  node_type = new TextNodeType();
  return node_type;
}

TextNode::TextNode(Scene* scene) : Node(scene, node_type) {}

TextNode::~TextNode() = default;

void TextNode::SetText(const std::string& val) {
  if (text_raw_ != val) {
    assert(Utils::IsValidUTF8(val));

    // In some cases we want to make sure this is a valid resource-string
    // since catching the error here is much more useful than if we catch
    // it at draw-time.  However this is expensive so we only do it for debug
    // mode or if the string looks suspicious.
    bool do_format_check{};
    bool print_false_positives{};

    if (g_buildconfig.debug_build()) {
      do_format_check = true;
    } else {
      if (val.size() > 1 && val[0] == '{' && val[val.size() - 1] == '}') {
        // ok, its got bounds like json; now if its either missing quotes or a
        // colon then let's check it..
        if (!strstr(val.c_str(), "\"") || !strstr(val.c_str(), ":")) {
          do_format_check = true;
          // we wanna avoid doing this check when we don't have to..
          // so lets print if we get a false positive
          print_false_positives = true;
        }
      }
    }

    if (do_format_check) {
      bool valid;
      g_game->CompileResourceString(val, "setText format check", &valid);
      if (!valid) {
        BA_LOG_ONCE("Invalid resource string: '" + val + "' on node '" + label()
                    + "'");
        Python::PrintStackTrace();
      } else if (print_false_positives) {
        BA_LOG_ONCE("Got false positive for json check on '" + val + "'");
        Python::PrintStackTrace();
      }
    }
    text_translation_dirty_ = true;
    text_raw_ = val;
  }
}

void TextNode::SetBig(bool val) {
  big_ = val;
  text_group_dirty_ = true;
  text_width_dirty_ = true;
}

auto TextNode::GetHAlign() const -> std::string {
  if (h_align_ == HAlign::kLeft) {
    return "left";
  } else if (h_align_ == HAlign::kRight) {
    return "right";
  } else if (h_align_ == HAlign::kCenter) {
    return "center";
  } else {
    BA_LOG_ONCE("Error: Invalid h_align value in text-node: "
                + std::to_string(static_cast<int>(h_align_)));
    return "<invalid>";
  }
}

void TextNode::SetHAlign(const std::string& val) {
  text_group_dirty_ = true;
  if (val == "left") {
    h_align_ = HAlign::kLeft;
  } else if (val == "right") {
    h_align_ = HAlign::kRight;
  } else if (val == "center") {
    h_align_ = HAlign::kCenter;
  } else {
    throw Exception("Invalid h_align for text node: " + val);
  }
}

auto TextNode::GetVAlign() const -> std::string {
  if (v_align_ == VAlign::kTop) {
    return "top";
  } else if (v_align_ == VAlign::kBottom) {
    return "bottom";
  } else if (v_align_ == VAlign::kCenter) {
    return "center";
  } else if (v_align_ == VAlign::kNone) {
    return "none";
  } else {
    BA_LOG_ONCE("Error: Invalid v_align value in text-node: "
                + std::to_string(static_cast<int>(v_align_)));
    return "<invalid>";
  }
}

void TextNode::SetVAlign(const std::string& val) {
  text_group_dirty_ = true;
  if (val == "top") {
    v_align_ = VAlign::kTop;
  } else if (val == "bottom") {
    v_align_ = VAlign::kBottom;
  } else if (val == "center") {
    v_align_ = VAlign::kCenter;
  } else if (val == "none") {
    v_align_ = VAlign::kNone;
  } else {
    throw Exception("Invalid v_align for text node: " + val);
  }
}

auto TextNode::GetHAttach() const -> std::string {
  if (h_attach_ == HAttach::kLeft) {
    return "left";
  } else if (h_attach_ == HAttach::kRight) {
    return "right";
  } else if (h_attach_ == HAttach::kCenter) {
    return "center";
  } else {
    BA_LOG_ONCE("Error: Invalid h_attach value in text-node: "
                + std::to_string(static_cast<int>(h_attach_)));
    return "<invalid>";
  }
}

void TextNode::SetHAttach(const std::string& val) {
  position_final_dirty_ = true;
  if (val == "left") {
    h_attach_ = HAttach::kLeft;
  } else if (val == "right") {
    h_attach_ = HAttach::kRight;
  } else if (val == "center") {
    h_attach_ = HAttach::kCenter;
  } else {
    throw Exception("Invalid h_attach for text node: " + val);
  }
}

auto TextNode::GetVAttach() const -> std::string {
  if (v_attach_ == VAttach::kTop) {
    return "top";
  } else if (v_attach_ == VAttach::kBottom) {
    return "bottom";
  } else if (v_attach_ == VAttach::kCenter) {
    return "center";
  } else {
    BA_LOG_ONCE("Error: Invalid v_attach value in text-node: "
                + std::to_string(static_cast<int>(v_attach_)));
    return "<invalid>";
  }
}

void TextNode::SetVAttach(const std::string& val) {
  position_final_dirty_ = true;
  if (val == "top") {
    v_attach_ = VAttach::kTop;
  } else if (val == "bottom") {
    v_attach_ = VAttach::kBottom;
  } else if (val == "center") {
    v_attach_ = VAttach::kCenter;
  } else {
    throw Exception("Invalid v_attach for text node: " + val);
  }
}

void TextNode::SetColor(const std::vector<float>& vals) {
  if (vals.size() != 3 && vals.size() != 4) {
    throw Exception("Expected float array of size 3 or 4 for color");
  }
  color_ = vals;
  if (color_.size() == 3) {
    color_.push_back(1.0f);
  }
}

void TextNode::SetTrailColor(const std::vector<float>& vals) {
  if (vals.size() != 3) {
    throw Exception("Expected float array of size 3 for trailcolor");
  }
  trail_color_ = vals;
}

void TextNode::SetPosition(const std::vector<float>& val) {
  if (val.size() != 2 && val.size() != 3) {
    throw Exception("Expected float array of length 2 or 3 for position; got "
                    + std::to_string(val.size()));
  }
  position_ = val;
  position_final_dirty_ = true;
}

void TextNode::OnScreenSizeChange() { position_final_dirty_ = true; }

void TextNode::Update() {
  // Update our final translate if need be.
  if (position_final_dirty_) {
    float offset_h;
    float offset_v;

    if (in_world_) {
      offset_h = 0.0f;
      offset_v = 0.0f;
    } else {
      // Screen space; apply alignment and stuff.
      if (h_attach_ == HAttach::kLeft) {
        offset_h = 0;
      } else if (h_attach_ == HAttach::kRight) {
        offset_h = g_graphics->screen_virtual_width();
      } else if (h_attach_ == HAttach::kCenter) {
        offset_h = g_graphics->screen_virtual_width() / 2;
      } else {
        throw Exception("invalid h_attach");
      }
      if (v_attach_ == VAttach::kTop) {
        offset_v = g_graphics->screen_virtual_height();
      } else if (v_attach_ == VAttach::kBottom) {
        offset_v = 0;
      } else if (v_attach_ == VAttach::kCenter) {
        offset_v = g_graphics->screen_virtual_height() / 2;
      } else {
        throw Exception("invalid v_attach");
      }
    }
    position_final_ = position_;
    if (position_final_.size() == 2) {
      position_final_.push_back(0.0f);
    }
    position_final_[0] += offset_h;
    position_final_[1] += offset_v;
    position_final_dirty_ = false;
  }
}

void TextNode::Draw(FrameDef* frame_def) {
  if (client_only_ && context().GetHostSession()) {
    return;
  }
  if (host_only_ && !context().GetHostSession()) {
    return;
  }

  // Apply subs/resources to get our actual text if need be.
  if (text_translation_dirty_) {
    text_translated_ =
        g_game->CompileResourceString(text_raw_, "TextNode::OnDraw");
    text_translation_dirty_ = false;
    text_group_dirty_ = true;
    text_width_dirty_ = true;
  }

  if (text_translated_.size() <= 0.0f) {
    return;
  }

  // recalc our text width if need be..
  if (text_width_dirty_) {
    text_width_ =
        g_text_graphics->GetStringWidth(text_translated_.c_str(), big_);
    text_width_dirty_ = false;
  }

  bool vr_2d_text = (IsVRMode() && !in_world_);

  // in vr mode we use the fixed overlay position if our scene is set for
  // that
  bool vr_use_fixed = (IsVRMode() && scene()->use_fixed_vr_overlay());

  // FIXME - in VR, fixed and front are currently mutually exclusive; need to
  // implement that.
  if (front_) {
    vr_use_fixed = false;
  }

  // make sure we're up to date
  Update();
  RenderPass& pass(*(in_world_
                         ? frame_def->overlay_3d_pass()
                         : (vr_use_fixed ? frame_def->GetOverlayFixedPass()
                            : front_     ? frame_def->overlay_front_pass()
                                         : frame_def->overlay_pass())));
  if (big_) {
    if (text_group_dirty_) {
      TextMesh::HAlign h_align;
      switch (h_align_) {
        case HAlign::kLeft:
          h_align = TextMesh::HAlign::kLeft;
          break;
        case HAlign::kRight:
          h_align = TextMesh::HAlign::kRight;
          break;
        case HAlign::kCenter:
          h_align = TextMesh::HAlign::kCenter;
          break;
        default:
          throw Exception();
      }

      TextMesh::VAlign v_align;
      switch (v_align_) {
        case VAlign::kNone:
          v_align = TextMesh::VAlign::kNone;
          break;
        case VAlign::kCenter:
          v_align = TextMesh::VAlign::kCenter;
          break;
        case VAlign::kTop:
          v_align = TextMesh::VAlign::kTop;
          break;
        case VAlign::kBottom:
          v_align = TextMesh::VAlign::kBottom;
          break;
        default:
          throw Exception();
      }

      // update if need be
      text_group_.SetText(text_translated_, h_align, v_align, true, 2.5f);
      text_group_dirty_ = false;
    }

    float z = vr_2d_text ? 0.0f : g_graphics->overlay_node_z_depth();

    assert(!text_width_dirty_);
    float tx = position_final_[0];
    float ty = position_final_[1];
    float tx_tilt = 0;
    float ty_tilt = 0;

    // left/rigth shift from tilting the device
    if (tilt_translate_ != 0.0f) {
      Vector3f tilt = g_graphics->tilt();
      tx_tilt = -tilt.y * tilt_translate_;
      ty_tilt = tilt.x * tilt_translate_;
    }

    assert(!text_width_dirty_);
    float extrascale;
    float textWidth = text_width_;
    float extrascale_2 = 3.5f;

    if (max_width_ > 0.0f && (textWidth * scale_ * extrascale_2) > max_width_) {
      extrascale = max_width_ / (textWidth * scale_ * extrascale_2);
    } else {
      extrascale = 1.0f;
    }
    extrascale *= scale_;

    float pass_width = pass.virtual_width();
    float pass_height = pass.virtual_height();
    {
      // new style
      {
        // draw trails..
        if (trail_) {
          int passes = 2;
          if (trail_project_scale_ != project_scale_) {
            for (int i = 0; i < passes; i++) {
              float o = trail_opacity_ * 0.5f;
              {
                auto i_f = static_cast<float>(i);
                auto passes_f = static_cast<float>(passes);
                float x = tx + tx_tilt * (i_f / passes_f) - pass_width / 2.0f;
                float y = ty + ty_tilt * (i_f / passes_f) - pass_height / 2.0f;
                float project_scale =
                    (trail_project_scale_
                     + static_cast<float>(i)
                           * (project_scale_ - trail_project_scale_)
                           / passes_f);
                SimpleComponent c(&pass);
                c.SetTransparent(true);
                c.SetPremultiplied(true);
                c.SetColor(trail_color_[0] * o, trail_color_[1] * o,
                           trail_color_[2] * o, 0.0f);
                c.setGlow(1.0f, 3.0f);

                // FIXME FIXME FIXME NEED A WAY TO BLUR IN THE SHADER
                int elem_count = text_group_.GetElementCount();
                for (int e = 0; e < elem_count; e++) {
                  // gracefully skip unloaded textures..
                  TextureData* t = text_group_.GetElementTexture(e);
                  if (!t->preloaded()) continue;
                  c.SetTexture(t);
                  c.SetMaskUV2Texture(text_group_.GetElementMaskUV2Texture(e));
                  c.PushTransform();
                  if (vr_2d_text) {
                    c.Translate(
                        0, 0,
                        vr_depth_ - 15.0f * static_cast<float>(passes - i));
                  }

                  // Fudge factors to keep our old look.. ew.
                  c.Translate(pass_width / 2 + 7.0f, pass_height / 2 + 35.0f,
                              z);
                  c.Scale(project_scale, project_scale);
                  c.Translate(x, y + 70.0f, 0);
                  c.Scale(extrascale * extrascale_2, extrascale * extrascale_2);
                  c.DrawMesh(text_group_.GetElementMesh(e));
                  c.PopTransform();
                }
                c.Submit();
              }
            }
          }
        }

        SimpleComponent c(&pass);
        c.SetTransparent(true);
        c.SetColor(color_[0], color_[1], color_[2], color_[3] * opacity_);

        int elem_count = text_group_.GetElementCount();
        bool did_submit = false;
        for (int e = 0; e < elem_count; e++) {
          // Gracefully skip unloaded textures.
          TextureData* t = text_group_.GetElementTexture(e);
          if (!t->preloaded()) continue;
          c.SetTexture(t);
          float shadow_opacity = shadow_;
          if (opacity_scales_shadow_) {
            float o = color_[3] * opacity_;
            shadow_opacity *= o * o;
          }
          c.SetShadow(-0.002f * text_group_.GetElementUScale(e),
                      -0.002f * text_group_.GetElementVScale(e), 2.5f,
                      shadow_opacity);
          if (shadow_opacity > 0) {
            c.SetMaskUV2Texture(text_group_.GetElementMaskUV2Texture(e));
          } else {
            c.clearMaskUV2Texture();
          }

          c.PushTransform();
          if (vr_2d_text) {
            c.Translate(0, 0, vr_depth_);
          }

          // Fudge factors to keep our old look.. ew.
          c.Translate(pass_width / 2 + 7.0f, pass_height / 2 + 35.0f, z);
          c.Scale(project_scale_, project_scale_);
          c.Translate(tx + tx_tilt - pass_width / 2,
                      ty + ty_tilt - pass_height / 2 + 70.0f, 0);
          c.Scale(extrascale * extrascale_2, extrascale * extrascale_2);
          c.DrawMesh(text_group_.GetElementMesh(e));
          c.PopTransform();
          // Any reason why we submit inside the loop here but not further
          // down?
          c.Submit();
          did_submit = true;
        }
        if (!did_submit) {
          // Make sure we've got at least one.
          c.Submit();
        }
      }
    }
  } else {
    // small text
    if (text_group_dirty_) {
      TextMesh::HAlign h_align;
      switch (h_align_) {
        case HAlign::kLeft:
          h_align = TextMesh::HAlign::kLeft;
          break;
        case HAlign::kRight:
          h_align = TextMesh::HAlign::kRight;
          break;
        case HAlign::kCenter:
          h_align = TextMesh::HAlign::kCenter;
          break;
        default:
          throw Exception();
      }

      TextMesh::VAlign v_align;
      switch (v_align_) {
        case VAlign::kNone:
          v_align = TextMesh::VAlign::kNone;
          break;
        case VAlign::kCenter:
          v_align = TextMesh::VAlign::kCenter;
          break;
        case VAlign::kTop:
          v_align = TextMesh::VAlign::kTop;
          break;
        case VAlign::kBottom:
          v_align = TextMesh::VAlign::kBottom;
          break;
        default:
          throw Exception();
      }

      // Update if need be.
      text_group_.SetText(text_translated_, h_align, v_align);
      text_group_dirty_ = false;
    }
    float z = vr_2d_text ? 0.0f
                         : (in_world_ ? position_final_[2]
                                      : g_graphics->overlay_node_z_depth());

    assert(!text_width_dirty_);
    float extrascale;
    if (max_width_ > 0.0f && text_width_ > max_width_) {
      extrascale = max_width_ / text_width_;
    } else {
      extrascale = 1.0f;
    }

    SimpleComponent c(&pass);
    c.SetTransparent(true);
    float fin_a = color_[3] * opacity_;
    int elem_count = text_group_.GetElementCount();
    for (int e = 0; e < elem_count; e++) {
      // Gracefully skip unloaded textures.
      TextureData* t = text_group_.GetElementTexture(e);
      if (!t->preloaded()) continue;
      c.SetTexture(t);
      float shadow_opacity = shadow_;
      if (opacity_scales_shadow_) {
        float o = color_[3] * opacity_;
        shadow_opacity *= o * o;
      }
      c.SetShadow(-0.004f * text_group_.GetElementUScale(e),
                  -0.004f * text_group_.GetElementVScale(e), 0.0f,
                  shadow_opacity);
      if (shadow_opacity > 0) {
        c.SetMaskUV2Texture(text_group_.GetElementMaskUV2Texture(e));
      } else {
        c.clearMaskUV2Texture();
      }
      if (text_group_.GetElementCanColor(e)) {
        c.SetColor(color_[0], color_[1], color_[2], fin_a);
      } else {
        c.SetColor(1, 1, 1, fin_a);
      }
      if (IsVRMode()) {
        c.SetFlatness(text_group_.GetElementMaxFlatness(e));
      } else {
        c.SetFlatness(
            std::min(text_group_.GetElementMaxFlatness(e), flatness_));
      }
      c.PushTransform();
      if (vr_2d_text) {
        c.Translate(0, 0, vr_depth_);
      }
      c.Translate(position_final_[0], position_final_[1], z);
      if (rotate_ != 0.0f) c.Rotate(rotate_, 0, 0, 1);
      c.Scale(scale_ * extrascale, scale_ * extrascale, 1.0f * extrascale);
      c.DrawMesh(text_group_.GetElementMesh(e));
      c.PopTransform();
    }
    c.Submit();
  }
}

void TextNode::OnLanguageChange() {
  // All we do here is mark our translated text dirty so it'll get remade at the
  // next draw.
  text_translation_dirty_ = true;
}

}  // namespace ballistica
