#include "ui/MindMapUi.h"

#include "app/DocumentSessionService.h"
#include "core/ImportService.h"
#include "ui/MindMapCanvasView.h"
#include "ui/NodeEditOverlay.h"
#include "ui/ShortcutRegistry.h"
#include "ui/UiCommandDispatcher.h"
#include "ui/branch/BranchStyle.h"
#include "ui/canvas/CanvasMath.h"
#include "ui/canvas/MindMapCanvasNodeMutators.h"
#include "ui/commands/CollapseNodeCommand.h"
#include "ui/commands/CommandHistory.h"
#include "ui/commands/UpdateEdgeLabelCommand.h"

#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <cassert>
#include <optional>
#include <string>

namespace mind_map::ui {

namespace {

constexpr float kMinZoom = 0.35F;
constexpr float kMaxZoom = 3.0F;
constexpr float kZoomStep = 0.1F;
constexpr float kStatusBarHeight = 26.0F;
constexpr float kFileDialogWidth = 600.0F;
constexpr float kFileDialogHeight = 400.0F;
constexpr float kEdgeLabelInputWidth = 180.0F;

// Returns true when the caller should proceed immediately; when false the action is queued behind
// the unsaved-changes guard modal which will invoke it after Save or Discard.
[[nodiscard]] bool TryNavigate(PendingNavAction action, UiState& state,
                                const mind_map::app::DocumentSessionService& session) {
  if (!session.IsDirty()) { return true; }
  state.pending_nav_ = action;
  return false;
}

void ExecutePendingNav(UiState& state, mind_map::app::DocumentSessionService& session,
                       commands::CommandHistory& history) {
  switch (state.pending_nav_) {
    case PendingNavAction::New: {
      mind_map::core::MindMapDocument dummy;
      session.New(dummy);
      history.Clear();
      state.canvas_.LoadFrom(dummy);
      state.ApplyViewport(dummy.viewport_);
      break;
    }
    case PendingNavAction::OpenDialog: {
      IGFD::FileDialogConfig cfg;
      cfg.path = ".";
      ImGuiFileDialog::Instance()->OpenDialog("OpenFileDlg", "Open Mind Map", ".mmh,.imx", cfg);
      break;
    }
    case PendingNavAction::ImportDialog: {
      IGFD::FileDialogConfig cfg;
      cfg.path = ".";
      ImGuiFileDialog::Instance()->OpenDialog("ImportFileDlg", "Import Mind Map", ".imx", cfg);
      break;
    }
    case PendingNavAction::None:
      break;
  }
  state.pending_nav_ = PendingNavAction::None;
}

void HandleMindMapKeyboardShortcuts(const ImGuiIO& io, const UiCommandDispatcher& dispatcher,
                                    UiState& state,
                                    mind_map::app::DocumentSessionService& session) {
  for (const ShortcutDef& def : kShortcuts) {
    if (!def.want_text_input_exempt_ && io.WantTextInput) {
      continue;
    }
    if (!IsTriggered(def, io)) {
      continue;
    }
    dispatcher.Dispatch(def.command_id_, state, session);
  }
}

void RenderEditMenu(const UiCommandDispatcher& dispatcher, UiState& state,
                    mind_map::app::DocumentSessionService& session,
                    const commands::CommandHistory& history) {
  if (ImGui::MenuItem("Undo", FormatLabel(FindShortcut(ShortcutAction::Undo)).c_str(),
                      /*selected=*/false, history.CanUndo())) {
    dispatcher.Dispatch(UiCommandId::Undo, state, session);
  }
  if (ImGui::MenuItem("Redo", FormatLabel(FindShortcut(ShortcutAction::Redo)).c_str(),
                      /*selected=*/false, history.CanRedo())) {
    dispatcher.Dispatch(UiCommandId::Redo, state, session);
  }
  ImGui::Separator();
  if (ImGui::MenuItem("Insert Child Node", FormatLabel(FindShortcut(ShortcutAction::InsertChildNode)).c_str())) {
    dispatcher.Dispatch(UiCommandId::InsertChildNode, state, session);
  }
  const std::optional<size_t> sel = state.canvas_.GetSelectedChildForBranchStyle();
  if (const bool can_delete = sel.has_value() && *sel > 0U;  // root (index 0) cannot be deleted
      ImGui::MenuItem("Delete Node", FormatLabel(FindShortcut(ShortcutAction::DeleteNode)).c_str(),
                      /*selected=*/false, can_delete)) {
    dispatcher.Dispatch(UiCommandId::DeleteNode, state, session);
  }
  ImGui::Separator();
  if (const bool can_paste = state.canvas_.GetSelectedNode().has_value();
      ImGui::MenuItem("Paste", FormatLabel(FindShortcut(ShortcutAction::Paste)).c_str(),
                      /*selected=*/false, can_paste)) {
    dispatcher.Dispatch(UiCommandId::Paste, state, session);
  }
  ImGui::Separator();
  const auto collapse_sel = state.canvas_.GetSelectedNode();
  const bool can_toggle_collapse = collapse_sel.has_value() && state.canvas_.NodeHasChildren(*collapse_sel);
  const char* const toggle_label = (can_toggle_collapse && state.canvas_.IsCollapsed(*collapse_sel))
      ? "Expand Node" : "Collapse Node";
  if (ImGui::MenuItem(toggle_label, FormatLabel(FindShortcut(ShortcutAction::CollapseNode)).c_str(),
                      /*selected=*/false, can_toggle_collapse)) {
    dispatcher.Dispatch(UiCommandId::ToggleCollapsed, state, session);
  }
}

void RenderSelectedIncomingEdgeStyleSelector(MindMapCanvasView& canvas,
                                             mind_map::app::DocumentSessionService& session,
                                             commands::CommandHistory& history) {
  if (!canvas.HasSelectedIncomingEdgeStyleTarget()) {
    ImGui::TextDisabled("Incoming edge style: select a non-root node on the canvas.");
    return;
  }
  const char* const node_label = canvas.GetSelectedIncomingEdgeChildLabel();
  assert(node_label != nullptr);
  ImGui::Text("Incoming edge to \"%s\"", node_label);
  ImGui::SameLine();
  const mind_map::ui::branch::BranchStyle current = canvas.GetBranchStyleForSelectedChildEdge();
  if (const char* const preview = mind_map::ui::branch::GetBranchStyleDisplayName(current);
      ImGui::BeginCombo("##SelectedIncomingEdgeBranchStyle", preview)) {
    for (int i = 0; i < mind_map::ui::branch::kBranchStyleCount; ++i) {
      const mind_map::ui::branch::BranchStyle style = mind_map::ui::branch::BranchStyleFromIndex(i);
      const bool selected = (style == current);
      if (ImGui::Selectable(mind_map::ui::branch::GetBranchStyleDisplayName(style), selected)) {
        canvas.SetBranchStyleForSelectedChildEdge(style);
        session.MarkDirty();
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // Edge label text field — synced from the canvas whenever the selection changes.
  static std::string s_edge_label_buf;
  static std::optional<size_t> s_last_edge_idx;
  const std::optional<size_t> edge_idx = canvas.GetSelectedChildForBranchStyle();
  if (s_last_edge_idx != edge_idx) {
    s_edge_label_buf = edge_idx.has_value() ? canvas::GetEdgeLabel(canvas, *edge_idx) : std::string{};
    s_last_edge_idx = edge_idx;
  }
  ImGui::SameLine();
  ImGui::TextUnformatted("Label:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(kEdgeLabelInputWidth);
  ImGui::InputText("##EdgeLabel", &s_edge_label_buf);
  if (ImGui::IsItemDeactivatedAfterEdit() && edge_idx.has_value()) {
    std::string old_label = canvas::GetEdgeLabel(canvas, *edge_idx);
    if (old_label != s_edge_label_buf) {
      history.Push(std::make_unique<commands::UpdateEdgeLabelCommand>(
          canvas, *edge_idx, std::move(old_label), s_edge_label_buf));
      session.MarkDirty();
    }
  }
}

void RenderBranchStyleSelector(MindMapCanvasView& canvas,
                               mind_map::app::DocumentSessionService& session) {
  if (const char* const preview = canvas.GetBranchStyleComboPreviewLabel();
      !ImGui::BeginCombo("Set all branches to", preview)) {
    return;
  }

  const bool uniform = canvas.BranchStylesAreUniform();
  const mind_map::ui::branch::BranchStyle representative = canvas.RepresentativeChildEdgeStyle();

  for (int i = 0; i < mind_map::ui::branch::kBranchStyleCount; ++i) {
    const mind_map::ui::branch::BranchStyle style = mind_map::ui::branch::BranchStyleFromIndex(i);
    const bool selected = uniform && (style == representative);
    if (ImGui::Selectable(mind_map::ui::branch::GetBranchStyleDisplayName(style), selected)) {
      canvas.SetBranchStyleForAllEdges(style);
      session.MarkDirty();
    }
    if (selected) {
      ImGui::SetItemDefaultFocus();
    }
  }
  ImGui::EndCombo();
}

void OpenSaveAsDialog(const mind_map::app::DocumentSessionService& session) {
  IGFD::FileDialogConfig cfg;
  cfg.path = ".";
  cfg.fileName = "untitled.mmh";
  if (session.HasPath()) {
    cfg.filePathName = std::string{session.GetCurrentPath()};
  }
  ImGuiFileDialog::Instance()->OpenDialog("SaveAsFileDlg", "Save Mind Map As", ".mmh", cfg);
}

void HandleSaveMenuItem(const UiState& state, mind_map::app::DocumentSessionService& session) {
  if (!session.HasPath()) {
    OpenSaveAsDialog(session);
    return;
  }
  const auto doc = state.canvas_.ToDocument(state.ToViewport());
  if (!session.Save(doc)) {
    // Save failed; repository already logged the error to stderr.
  }
}

void RenderFileMenu(const UiCommandDispatcher& dispatcher, UiState& state,
                    mind_map::app::DocumentSessionService& session,
                    commands::CommandHistory& history) {
  if (ImGui::MenuItem("New") && TryNavigate(PendingNavAction::New, state, session)) {
    mind_map::core::MindMapDocument dummy;
    session.New(dummy);
    history.Clear();
    state.canvas_.LoadFrom(dummy);
    state.ApplyViewport(dummy.viewport_);
  }
  if (ImGui::MenuItem("Open...", "Cmd+O") && TryNavigate(PendingNavAction::OpenDialog, state, session)) {
    IGFD::FileDialogConfig cfg;
    cfg.path = ".";
    ImGuiFileDialog::Instance()->OpenDialog("OpenFileDlg", "Open Mind Map", ".mmh,.imx", cfg);
  }
  if (ImGui::MenuItem("New Window")) {
    state.pending_launch_path_ = "";
  }
  if (ImGui::MenuItem("Open in New Window...")) {
    IGFD::FileDialogConfig cfg;
    cfg.path = ".";
    ImGuiFileDialog::Instance()->OpenDialog("OpenInNewWindowDlg", "Open in New Window", ".mmh,.imx", cfg);
  }
  ImGui::Separator();
  if (ImGui::MenuItem("Save", "Cmd+S")) {
    HandleSaveMenuItem(state, session);
  }
  if (ImGui::MenuItem("Save As...", "Cmd+Shift+S")) {
    OpenSaveAsDialog(session);
  }
  ImGui::Separator();
  if (ImGui::MenuItem("Import...", "Cmd+Shift+O") && TryNavigate(PendingNavAction::ImportDialog, state, session)) {
    IGFD::FileDialogConfig cfg;
    cfg.path = ".";
    ImGuiFileDialog::Instance()->OpenDialog("ImportFileDlg", "Import Mind Map", ".imx", cfg);
  }
  ImGui::Separator();
  if (ImGui::MenuItem("Reset Layout")) {
    dispatcher.Dispatch(UiCommandId::ResetLayout, state, session);
  }
  if (ImGui::MenuItem("Reset View")) {
    dispatcher.Dispatch(UiCommandId::ResetView, state, session);
  }
}

void RenderMainMenuBar(const UiCommandDispatcher& dispatcher, UiState& state,
                       mind_map::app::DocumentSessionService& session,
                       commands::CommandHistory& history) {
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  if (ImGui::BeginMenu("File")) {
    RenderFileMenu(dispatcher, state, session, history);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Edit")) {
    RenderEditMenu(dispatcher, state, session, history);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    if (ImGui::MenuItem("Zoom In", "Cmd+=")) {
      dispatcher.Dispatch(UiCommandId::ZoomIn, state, session);
    }
    if (ImGui::MenuItem("Zoom Out", "Cmd+-")) {
      dispatcher.Dispatch(UiCommandId::ZoomOut, state, session);
    }
    if (ImGui::MenuItem("Reset View", "Cmd+0")) {
      dispatcher.Dispatch(UiCommandId::ResetView, state, session);
    }
    if (ImGui::MenuItem("Show Status Bar", nullptr, state.show_status_bar_)) {
      dispatcher.Dispatch(UiCommandId::ToggleStatusBar, state, session);
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Help")) {
    ImGui::MenuItem("Guide map: run MindMapHelper with --help (command line).", nullptr, false, false);
    ImGui::MenuItem("Non-root node: select to edit incoming edge style; root clears selection.", nullptr, false, false);
    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void HandleCanvasZoom(const ImGuiIO& io, bool canvas_hovered, ImVec2 canvas_p0, ImVec2& pan_px,
                      float& zoom) {
  if (!canvas_hovered || io.MouseWheel == 0.0F) {
    return;
  }
  const float previous_zoom = zoom;
  zoom = std::clamp(zoom + (io.MouseWheel * kZoomStep), kMinZoom, kMaxZoom);
  const ImVec2 world_under_cursor = mind_map::canvas::ScreenToWorld(io.MousePos, canvas_p0, pan_px, previous_zoom);
  pan_px.x += world_under_cursor.x * (previous_zoom - zoom);
  pan_px.y += world_under_cursor.y * (previous_zoom - zoom);
}

void HandleCanvasPointerInput(bool canvas_item_active, const ImGuiIO& io,
                              const MindMapPointerState& pointer_state, ImVec2& pan_px,
                              MindMapCanvasView& canvas,
                              mind_map::app::DocumentSessionService& session,
                              commands::CommandHistory& history) {
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && pointer_state.canvas_hovered_) {
    canvas.OnPrimaryDown(pointer_state);
    if (auto target = canvas.ConsumeCollapseToggleTarget(); target.has_value()) {
      const bool collapsing = !canvas.IsCollapsed(*target);
      history.Push(std::make_unique<commands::CollapseNodeCommand>(canvas, *target, collapsing));
      session.MarkDirty();
    }
  }

  if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && pointer_state.canvas_hovered_) {
    if (const auto sel = canvas.GetSelectedNode(); sel.has_value()) {
      canvas.BeginEditing(*sel);
    }
  }

  if (canvas_item_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    if (canvas.IsDraggingContent()) {
      canvas.OnPrimaryDrag(pointer_state);
      session.MarkDirty();
    }
    else {
      pan_px.x += io.MouseDelta.x;
      pan_px.y += io.MouseDelta.y;
    }
  }

  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    canvas.OnPrimaryUp();
  }
}

void RenderCanvas(UiState& state, mind_map::app::DocumentSessionService& session,
                  commands::CommandHistory& history) {
  const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
  const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
  const ImVec2 canvas_p1 = {canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y};
  state.canvas_p0_ = canvas_p0;
  state.canvas_sz_ = canvas_sz;

  ImGui::InvisibleButton("mind_map_canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft);
  const bool canvas_hovered = ImGui::IsItemHovered();
  const bool canvas_item_active = ImGui::IsItemActive();
  const ImGuiIO& io = ImGui::GetIO();

  HandleCanvasZoom(io, canvas_hovered, canvas_p0, state.pan_px_, state.zoom_);

  const ImVec2 mouse_world = mind_map::canvas::ScreenToWorld(io.MousePos, canvas_p0, state.pan_px_, state.zoom_);
  MindMapPointerState pointer_state = {};
  pointer_state.mouse_screen_ = io.MousePos;
  pointer_state.mouse_world_ = mouse_world;
  pointer_state.canvas_hovered_ = canvas_hovered;
  pointer_state.zoom_ = state.zoom_;
  HandleCanvasPointerInput(canvas_item_active, io, pointer_state, state.pan_px_,
                           state.canvas_, session, history);

  MindMapCanvasRenderContext render_context = {};
  render_context.draw_list_ = ImGui::GetWindowDrawList();
  render_context.canvas_p0_ = canvas_p0;
  render_context.canvas_p1_ = canvas_p1;
  render_context.pan_px_ = state.pan_px_;
  render_context.zoom_ = state.zoom_;
  render_context.canvas_hovered_ = canvas_hovered;
  render_context.mouse_world_ = mouse_world;

  render_context.draw_list_->PushClipRect(canvas_p0, canvas_p1, true);
  state.canvas_.Render(render_context);
  render_context.draw_list_->PopClipRect();

  RenderNodeEditOverlay(render_context, state.canvas_, session, history);
}

void RenderStatusBar(const UiState& state) {
  if (!state.show_status_bar_) {
    return;
  }
  ImGui::Separator();
  const char* const edge_child_label = state.canvas_.GetSelectedIncomingEdgeChildLabel();
  if (edge_child_label != nullptr) {
    ImGui::Text(
        "Status  Branches: %s  |  Edge to \"%s\": %s  |  Zoom: %.2f  |  Pan: (%.1f, %.1f)",
        state.canvas_.GetBranchStyleComboPreviewLabel(), edge_child_label,
        mind_map::ui::branch::GetBranchStyleDisplayName(state.canvas_.GetBranchStyleForSelectedChildEdge()),
        state.zoom_, state.pan_px_.x, state.pan_px_.y);
  }
  else {
    ImGui::Text("Status  Branches: %s  |  Selected edge: none  |  Zoom: %.2f  |  Pan: (%.1f, %.1f)",
                state.canvas_.GetBranchStyleComboPreviewLabel(), state.zoom_,
                state.pan_px_.x, state.pan_px_.y);
  }
}

void RenderNavGuardModal(UiState& state, mind_map::app::DocumentSessionService& session,
                         commands::CommandHistory& history) {
  const ImGuiViewport* const main_vp = ImGui::GetMainViewport();
  assert(main_vp != nullptr);
  ImGui::SetNextWindowPos(main_vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5F, 0.5F));
  if (!ImGui::BeginPopupModal("Unsaved Changes##nav", nullptr,
                              ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  ImGui::TextUnformatted("You have unsaved changes. What would you like to do?");
  ImGui::Separator();

  bool proceed = false;
  if (session.HasPath()) {
    if (ImGui::Button("Save")) {
      const auto doc = state.canvas_.ToDocument(state.ToViewport());
      if (session.Save(doc)) {
        proceed = true;
        ImGui::CloseCurrentPopup();
      }
    }
  } else {
    if (ImGui::Button("Save As...")) {
      state.save_as_context_ = SaveAsResumingContext::NavGuard;
      OpenSaveAsDialog(session);
      ImGui::CloseCurrentPopup();
    }
  }

  ImGui::SameLine();
  if (ImGui::Button("Discard")) {
    session.ClearDirty();
    proceed = true;
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    state.pending_nav_ = PendingNavAction::None;
    ImGui::CloseCurrentPopup();
  }

  ImGui::EndPopup();

  if (proceed) {
    ExecutePendingNav(state, session, history);
  }
}

void RenderCloseGuardModal(UiState& state, mind_map::app::DocumentSessionService& session) {
  const ImGuiViewport* const main_vp = ImGui::GetMainViewport();
  assert(main_vp != nullptr);
  ImGui::SetNextWindowPos(main_vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5F, 0.5F));
  if (!ImGui::BeginPopupModal("Unsaved Changes##close", nullptr,
                              ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  ImGui::TextUnformatted("You have unsaved changes. What would you like to do?");
  ImGui::Separator();

  if (session.HasPath()) {
    if (ImGui::Button("Save")) {
      const auto doc = state.canvas_.ToDocument(state.ToViewport());
      if (session.Save(doc)) {
        session.ConfirmClose();
        session.CancelClose();
        ImGui::CloseCurrentPopup();
      }
    }
  } else {
    if (ImGui::Button("Save As...")) {
      state.save_as_context_ = SaveAsResumingContext::CloseGuard;
      OpenSaveAsDialog(session);
      ImGui::CloseCurrentPopup();
    }
  }

  ImGui::SameLine();
  if (ImGui::Button("Discard")) {
    session.ConfirmClose();
    session.CancelClose();
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    session.CancelClose();
    ImGui::CloseCurrentPopup();
  }

  ImGui::EndPopup();
}

void ResumeSaveAsContext(UiState& state, mind_map::app::DocumentSessionService& session,
                         commands::CommandHistory& history, bool saved) {
  if (state.save_as_context_ == SaveAsResumingContext::NavGuard) {
    state.save_as_context_ = SaveAsResumingContext::None;
    if (saved) { ExecutePendingNav(state, session, history); }
  } else if (state.save_as_context_ == SaveAsResumingContext::CloseGuard) {
    state.save_as_context_ = SaveAsResumingContext::None;
    if (saved) { session.ConfirmClose(); session.CancelClose(); }
  }
}

void RenderFileDialogs(UiState& state, mind_map::app::DocumentSessionService& session,
                       commands::CommandHistory& history) {
  auto* const fd = ImGuiFileDialog::Instance();
  const ImVec2 dialog_min_size{kFileDialogWidth, kFileDialogHeight};
  const mind_map::core::ImportService import_service;

  if (fd->Display("ImportFileDlg", ImGuiWindowFlags_NoCollapse, dialog_min_size)) {
    if (fd->IsOk()) {
      const std::string path = fd->GetFilePathName();
      if (auto doc = import_service.ImportFile(path)) {
        session.ApplyImportedDocument();
        history.Clear();
        state.canvas_.LoadFrom(*doc);
        state.ApplyViewport(doc->viewport_);
      }
    }
    fd->Close();
  }

  if (fd->Display("OpenFileDlg", ImGuiWindowFlags_NoCollapse, dialog_min_size)) {
    if (fd->IsOk()) {
      const std::string path = fd->GetFilePathName();
      mind_map::core::MindMapDocument doc;
      if (session.OpenFromPath(path, doc, import_service)) {
        history.Clear();
        state.canvas_.LoadFrom(doc);
        state.ApplyViewport(doc.viewport_);
      }
    }
    fd->Close();
  }

  if (fd->Display("SaveAsFileDlg", ImGuiWindowFlags_NoCollapse, dialog_min_size)) {
    bool saved = false;
    if (fd->IsOk()) {
      const std::string path = fd->GetFilePathName();
      const auto doc = state.canvas_.ToDocument(state.ToViewport());
      saved = session.SaveAs(path, doc);
      if (!saved) {
        // Save failed; repository already logged the error to stderr.
      }
    }
    // If context is set, resume the action that triggered Save As (or let modal reopen on cancel).
    ResumeSaveAsContext(state, session, history, saved);
    fd->Close();
  }

  if (fd->Display("OpenInNewWindowDlg", ImGuiWindowFlags_NoCollapse, dialog_min_size)) {
    if (fd->IsOk()) {
      state.pending_launch_path_ = fd->GetFilePathName();
    }
    fd->Close();
  }
}

}  // namespace

void RenderMainUi(UiState& state, mind_map::app::DocumentSessionService& session,
                  commands::CommandHistory& history) {
  const UiCommandDispatcher command_dispatcher{history};
  HandleMindMapKeyboardShortcuts(ImGui::GetIO(), command_dispatcher, state, session);
  RenderMainMenuBar(command_dispatcher, state, session, history);

  const ImGuiViewport* const viewport = ImGui::GetMainViewport();
  assert(viewport != nullptr);
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
  const ImGuiWindowFlags root_flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoSavedSettings;
  ImGui::Begin("MindMap Helper", nullptr, root_flags);

  if (const float content_height = state.show_status_bar_ ? -kStatusBarHeight : 0.0F;
      ImGui::BeginChild("MindMapHelperContent", ImVec2(0.0F, content_height), ImGuiChildFlags_None,
                        ImGuiWindowFlags_None)) {
    RenderBranchStyleSelector(state.canvas_, session);
    ImGui::SameLine();
    if (ImGui::Button("Reset layout")) {
      command_dispatcher.Dispatch(UiCommandId::ResetLayout, state, session);
    }
    RenderSelectedIncomingEdgeStyleSelector(state.canvas_, session, history);
    ImGui::Text("Zoom %.2f", state.zoom_);
    RenderCanvas(state, session, history);
  }
  ImGui::EndChild();

  RenderStatusBar(state);

  const bool save_as_pending = state.save_as_context_ != SaveAsResumingContext::None;

  // Navigation guard: open modal when a destructive navigation was requested while dirty.
  if (state.pending_nav_ != PendingNavAction::None && !save_as_pending
      && !ImGui::IsPopupOpen("Unsaved Changes##nav")) {
    ImGui::OpenPopup("Unsaved Changes##nav");
  }
  RenderNavGuardModal(state, session, history);

  // Close guard: open modal once when AppMain requests a close while dirty.
  if (session.IsCloseRequested() && !save_as_pending
      && !ImGui::IsPopupOpen("Unsaved Changes##close")) {
    ImGui::OpenPopup("Unsaved Changes##close");
  }
  RenderCloseGuardModal(state, session);

  ImGui::End();
  ImGui::PopStyleVar(2);

  RenderFileDialogs(state, session, history);
}

}  // namespace mind_map::ui
