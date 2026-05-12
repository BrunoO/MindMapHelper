#!/usr/bin/env python3
"""
Extract mind map structure from an iMindMap / MindManager-style .imx (ZIP) archive.

Reads data.xml and mapmeta.xml from the archive, builds a tree from floatingIdea +
branch edges, identifies embedded / floating raster images, resolves branch and
node captions when a raster's parent is a ``<branch/>``, and writes JSON, plain
text, and OPML under the output directory.
"""

from __future__ import annotations

import argparse
import html
import json
import re
import sys
import zipfile
from pathlib import Path
from typing import Any, Dict, List, Optional, Set
import xml.etree.ElementTree as ET

CELL_TEXT_KEY = "com.thinkbuzan.gaia.cell.text"
HTML_LABEL_KEY = "com.thinkbuzan.gaia.entities.HTMLLabel"
CENTRAL_IMAGE_PROP_KEY = "com.thinkbuzan.phoenix.centralIdeaImageId"
TAG_STRIP = re.compile(r"<[^>]+>")
_IMAGE_RESOURCE_ID = re.compile(r"(?:^|;)image=(\d+)(?:;|$)")


def strip_html_tags(s: str) -> str:
    return TAG_STRIP.sub("", s)


def get_text(elem: Optional[ET.Element]) -> str:
    """Prefer cell.text property, else HTMLLabel inner text without tags."""
    if elem is None:
        return ""

    html_fallback = ""
    for node in elem.iter():
        if node.tag != "property":
            continue
        key = node.get("key")
        if key == CELL_TEXT_KEY:
            val_attr = node.get("value")
            if val_attr is not None:
                return val_attr.strip()
            text = (node.text or "").strip()
            if text:
                return text
        if key == HTML_LABEL_KEY and not html_fallback:
            html_label = node.find("HTMLLabel")
            if html_label is not None:
                inner = "".join(html_label.itertext())
                if inner:
                    html_fallback = strip_html_tags(inner).strip()

    return html_fallback


_STROKE_COLOR = re.compile(r"strokeColor=([#\w]+)")


def get_color(elem: Optional[ET.Element]) -> Optional[str]:
    if elem is None:
        return None
    style = elem.get("style")
    if not style:
        return None
    m = _STROKE_COLOR.search(style)
    return m.group(1) if m else None


def meta_child_value(meta_root: ET.Element, tag: str) -> str:
    el = meta_root.find(f".//{tag}")
    if el is None:
        return ""
    val = el.get("value")
    return val if val is not None else ""


def image_resource_id_from_style(style: str) -> Optional[str]:
    """Raster bundle id inside the imx zip (entry data/<id>), from style fragment image=<digits>."""
    if not style:
        return None
    m = _IMAGE_RESOURCE_ID.search(style)
    return m.group(1) if m else None


def mx_geometry_dict(elem: Optional[ET.Element]) -> Optional[Dict[str, str]]:
    if elem is None:
        return None
    geo = elem.find("mxGeometry")
    if geo is None:
        return None
    return {k: v for k, v in geo.attrib.items()}


def direct_property_map(elem: ET.Element) -> Dict[str, str]:
    """Immediate <property key=...> children under <properties>, if present."""
    props_el = elem.find("properties")
    if props_el is None:
        return {}
    out: Dict[str, str] = {}
    for pr in props_el.findall("property"):
        key = pr.get("key")
        if not key:
            continue
        val = pr.get("value")
        if val is None and (pr.text and pr.text.strip()):
            val = pr.text.strip()
        if val is not None:
            out[key] = val
    return out


def build_incoming_branch_label_by_target(data_root: ET.Element) -> Dict[str, str]:
    """
    Map branch endpoint id -> label text shown on the incoming connector.

    In IMX, ``branchNode`` topic titles are often stored on the ``<branch/>`` whose
    ``target`` is that node, not on the node element itself.
    """
    out: Dict[str, str] = {}
    for br in data_root.iter("branch"):
        tid = br.get("target")
        if not tid:
            continue
        if tid in out:
            continue
        label = get_text(br)
        if label:
            out[tid] = label
    return out


def node_caption_summary(
    node_id: Optional[str],
    elem_map: Dict[str, ET.Element],
    incoming_by_target: Dict[str, str],
) -> Dict[str, Any]:
    if not node_id:
        return {"id": None, "element": None, "caption": ""}
    node = elem_map.get(node_id)
    caption = incoming_by_target.get(node_id, "")
    if not caption and node is not None:
        caption = get_text(node)
    return {
        "id": node_id,
        "element": node.tag if node is not None else None,
        "caption": caption,
    }


def resolve_graph_link_for_parent(
    parent_id: Optional[str],
    elem_map: Dict[str, ET.Element],
    incoming_by_target: Dict[str, str],
) -> Optional[Dict[str, Any]]:
    """
    Interpret ``parent`` of a raster cell.

    Sketch / floating images are usually parented by id to a ``<branch/>``; the
    branch's ``source`` / ``target`` identify the linked graph endpoints.
    """
    if not parent_id:
        return None
    parent_el = elem_map.get(parent_id)
    if parent_el is None:
        return {
            "relation": "unknown_parent",
            "parent_id": parent_id,
        }
    if parent_el.tag == "branch":
        src = parent_el.get("source")
        tgt = parent_el.get("target")
        return {
            "relation": "branch",
            "branch_id": parent_id,
            "branch_edge_label": get_text(parent_el),
            "source": node_caption_summary(src, elem_map, incoming_by_target),
            "target": node_caption_summary(tgt, elem_map, incoming_by_target),
        }
    return {
        "relation": "other",
        "parent_element": parent_el.tag,
        "parent_id": parent_id,
    }


def format_graph_link_one_line(graph_link: Optional[Dict[str, Any]]) -> str:
    """Compact human-readable summary for TXT / OPML."""
    if not isinstance(graph_link, dict):
        return ""
    rel = graph_link.get("relation")
    if rel == "branch":
        src = graph_link.get("source") or {}
        tgt = graph_link.get("target") or {}
        bel = graph_link.get("branch_edge_label") or ""
        sc = src.get("caption") or src.get("id") or "?"
        tc = tgt.get("caption") or tgt.get("id") or "?"
        return f"branch {graph_link.get('branch_id')} '{bel}': {sc} -> {tc}"
    if rel == "other":
        return f"parent <{graph_link.get('parent_element')}> id={graph_link.get('parent_id')}"
    if rel == "unknown_parent":
        return f"missing parent id={graph_link.get('parent_id')}"
    return ""


def data_zip_paths_for_resource(zip_names: Set[str], resource_id: str) -> List[str]:
    forward = f"data/{resource_id}"
    if forward in zip_names:
        return [forward]
    back = f"data\\{resource_id}"
    if back in zip_names:
        return [back]
    return []


def guess_image_extension(data: bytes) -> str:
    if len(data) >= 3 and data[0:3] == b"\xff\xd8\xff":
        return ".jpg"
    if len(data) >= 8 and data[0:8] == b"\x89PNG\r\n\x1a\n":
        return ".png"
    if len(data) >= 6 and data[0:6] in (b"GIF87a", b"GIF89a"):
        return ".gif"
    if len(data) >= 12 and data[0:4] == b"RIFF" and data[8:12] == b"WEBP":
        return ".webp"
    return ".bin"


def collect_embedded_graphics(
    data_root: ET.Element,
    floating_idea: ET.Element,
    zip_names: Set[str],
) -> Dict[str, Any]:
    """
    Identify raster assets referenced from the graph (central topic + floating images).

    Embedded files typically live at data/<numeric_id> inside the imx zip.

    Raster cells may appear as ``<image/>`` (common in older exports) or as
    ``<appcell/>`` with ``shape=image;image=<id>`` (e.g. ThinkBuzan sketch cells).

    When ``parent`` references a ``<branch/>``, ``graph_link`` resolves ``source`` /
    ``target`` endpoints and captions (incoming branch labels).
    """
    central: Optional[Dict[str, Any]] = None
    fi_style = floating_idea.get("style") or ""
    fi_rid = image_resource_id_from_style(fi_style)
    fi_props = direct_property_map(floating_idea)
    central_uuid = fi_props.get(CENTRAL_IMAGE_PROP_KEY)
    if fi_rid or central_uuid:
        paths = data_zip_paths_for_resource(zip_names, fi_rid) if fi_rid else []
        central = {
            "element": "floatingIdea",
            "id": floating_idea.get("id"),
            "label_text": get_text(floating_idea),
            "image_resource_id": fi_rid,
            "central_idea_image_uuid": central_uuid if central_uuid else None,
            "zip_entries": paths,
            "present_in_archive": bool(paths),
            "geometry": mx_geometry_dict(floating_idea),
            "style": fi_style if fi_style else None,
        }

    elem_map: Dict[str, ET.Element] = {}
    for el in data_root.iter():
        eid = el.get("id")
        if eid:
            elem_map[eid] = el
    incoming_by_target = build_incoming_branch_label_by_target(data_root)

    floating_images: List[Dict[str, Any]] = []
    seen_cell_ids: Set[str] = set()

    def append_raster_cell(el: ET.Element, kind: str, element_tag: str) -> None:
        style = el.get("style") or ""
        rid = image_resource_id_from_style(style)
        if not rid:
            return
        cid = el.get("id")
        if cid:
            if cid in seen_cell_ids:
                return
            seen_cell_ids.add(cid)
        paths = data_zip_paths_for_resource(zip_names, rid) if rid else []
        props = direct_property_map(el)
        parent_id = el.get("parent")
        floating_images.append(
            {
                "kind": kind,
                "element": element_tag,
                "id": cid,
                "parent_cell_id": parent_id,
                "connectable": el.get("connectable"),
                "vertex": el.get("vertex"),
                "image_resource_id": rid,
                "zip_entries": paths,
                "present_in_archive": bool(paths),
                "geometry": mx_geometry_dict(el),
                "style": style if style else None,
                "properties": props if props else None,
                "graph_link": resolve_graph_link_for_parent(parent_id, elem_map, incoming_by_target),
            }
        )

    for el in data_root.iter("image"):
        append_raster_cell(el, "floating_image", "image")
    for el in data_root.iter("appcell"):
        append_raster_cell(el, "appcell_image", "appcell")

    return {
        "central_topic": central,
        "floating_images": floating_images,
    }


def export_embedded_image_files(
    zf: zipfile.ZipFile,
    zip_names: Set[str],
    out_dir: Path,
    graphics: Dict[str, Any],
) -> List[str]:
    """Copy data/<id> blobs from the imx zip into out_dir/embedded_images/. Returns written paths."""
    target_dir = out_dir / "embedded_images"
    target_dir.mkdir(parents=True, exist_ok=True)
    written: List[str] = []
    seen: Set[str] = set()

    def export_one(resource_id: Optional[str], prefix: str) -> None:
        if not resource_id or resource_id in seen:
            return
        paths = data_zip_paths_for_resource(zip_names, resource_id)
        if not paths:
            return
        zpath = paths[0]
        try:
            raw = zf.read(zpath)
        except KeyError:
            return
        seen.add(resource_id)
        ext = guess_image_extension(raw)
        name = f"{prefix}{resource_id}{ext}"
        dest = target_dir / name
        dest.write_bytes(raw)
        written.append(str(dest))

    central = graphics.get("central_topic")
    if isinstance(central, dict):
        export_one(central.get("image_resource_id"), "central_")

    for item in graphics.get("floating_images", []):
        if isinstance(item, dict):
            export_one(item.get("image_resource_id"), "floating_")

    return written


def build_tree(
    data_root: ET.Element,
    elem_map: Dict[str, ET.Element],
    elem_id: str,
    depth: int,
) -> Optional[Dict[str, Any]]:
    elem = elem_map.get(elem_id)
    if elem is None:
        return None

    text = get_text(elem)
    color = get_color(elem)

    node: Dict[str, Any] = {
        "text": text,
        "id": elem_id,
        "depth": depth,
        "children": [],
    }
    if color is not None:
        node["color"] = color

    for branch in data_root.findall(".//branch"):
        if branch.get("source") != elem_id:
            continue
        target_id = branch.get("target")
        if not target_id:
            continue
        branch_text = get_text(branch)
        child_node = build_tree(data_root, elem_map, target_id, depth + 1)
        if child_node is not None:
            if branch_text:
                child_node["branch"] = branch_text
            node["children"].append(child_node)

    return node


def build_text_lines(node: Dict[str, Any], indent: int = 0) -> List[str]:
    prefix = "  " * indent
    lines: List[str] = []
    branch = node.get("branch")
    if branch:
        lines.append(f"{prefix}- {branch}")
        lines.append(f"{prefix}  {node['text']}")
    else:
        lines.append(f"{prefix}{node['text']}")
    for child in node["children"]:
        lines.extend(build_text_lines(child, indent + 1))
    return lines


def graphics_summary_lines(graphics: Dict[str, Any]) -> List[str]:
    lines: List[str] = []
    lines.append("--- Embedded graphics ---")
    central = graphics.get("central_topic")
    if isinstance(central, dict):
        rid = central.get("image_resource_id")
        z = central.get("zip_entries") or []
        pres = central.get("present_in_archive")
        lines.append("Central topic (floatingIdea):")
        lines.append(f"  id: {central.get('id')}")
        lines.append(f"  label: {central.get('label_text')}")
        if central.get("central_idea_image_uuid"):
            lines.append(f"  central_idea_image_uuid: {central['central_idea_image_uuid']}")
        if rid:
            lines.append(f"  image_resource_id: {rid}  archive: {z}  present={pres}")
        else:
            lines.append("  (no image= raster id in style)")
    else:
        lines.append("Central topic: (no image metadata)")

    floating = graphics.get("floating_images") or []
    lines.append(f"Non-central raster cells ({len(floating)}):")
    if not floating:
        lines.append("  (none)")
    else:
        for fi in floating:
            rid = fi.get("image_resource_id")
            z = fi.get("zip_entries") or []
            lines.append(
                f"  - [{fi.get('kind', '?')}] id={fi.get('id')}  element=<{fi.get('element')}>  "
                f"parent={fi.get('parent_cell_id')}  resource={rid}  files={z}  present={fi.get('present_in_archive')}"
            )
            geo = fi.get("geometry")
            if geo:
                lines.append(f"      geometry: {geo}")
            gl = fi.get("graph_link")
            gl_line = format_graph_link_one_line(gl if isinstance(gl, dict) else None)
            if gl_line:
                lines.append(f"      {gl_line}")
    lines.append("")
    return lines


def opml_escape(text: str) -> str:
    return (
        html.escape(text, quote=True)
        .replace("\n", " ")
        .replace("\r", " ")
    )


def build_opml(node: Dict[str, Any]) -> str:
    if node.get("branch"):
        text = f"{node['branch']}: {node['text']}"
    else:
        text = str(node["text"])

    text = opml_escape(text)
    children: List[Dict[str, Any]] = node["children"]
    if not children:
        return f'<outline text="{text}" />'

    parts = [f'<outline text="{text}">']
    for child in children:
        parts.append(build_opml(child))
    parts.append("</outline>")
    return "\n".join(parts)


def build_opml_assets_section(graphics: Dict[str, Any]) -> str:
    """Second top-level outline under <body> for non-tree assets (OPML allows multiple outlines)."""
    outlines: List[str] = []
    central = graphics.get("central_topic")
    if isinstance(central, dict) and central.get("image_resource_id"):
        rid = central["image_resource_id"]
        label = central.get("label_text") or ""
        t = opml_escape(f"[Central image] data/{rid} — {label}")
        outlines.append(f'<outline text="{t}" />')

    for fi in graphics.get("floating_images") or []:
        if not isinstance(fi, dict):
            continue
        rid = fi.get("image_resource_id") or "?"
        pid = fi.get("parent_cell_id") or "?"
        kind = fi.get("kind") or "image"
        elname = fi.get("element") or "?"
        gl = fi.get("graph_link")
        extra = format_graph_link_one_line(gl if isinstance(gl, dict) else None)
        suffix = f" — {extra}" if extra else ""
        t = opml_escape(f"[{kind} <{elname}>] data/{rid} (cell {fi.get('id')}, parent {pid}){suffix}")
        outlines.append(f'<outline text="{t}" />')

    if not outlines:
        return ""

    inner = "\n    ".join(outlines)
    return f'<outline text="[Embedded graphics]">\n    {inner}\n  </outline>'


def extract_imx(imx_path: Path, output_dir: Path, export_image_files: bool) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    with zipfile.ZipFile(imx_path, "r") as zf:
        zip_names = set(zf.namelist())
        try:
            data_xml = zf.read("data.xml")
            meta_xml = zf.read("mapmeta.xml")
        except KeyError as exc:
            raise FileNotFoundError(f"Missing required entry in {imx_path}: {exc}") from exc

        data_root = ET.fromstring(data_xml)
        meta_root = ET.fromstring(meta_xml)

        title = meta_child_value(meta_root, "Title")
        author = meta_child_value(meta_root, "InitialAuthor")
        created = meta_child_value(meta_root, "CreationTime")

        print(f"Extracting mindmap: {title}")

        elem_map: Dict[str, ET.Element] = {}
        for el in data_root.iter():
            eid = el.get("id")
            if eid:
                elem_map[eid] = el

        root_el = data_root.find(".//floatingIdea")
        if root_el is None:
            raise ValueError("No floatingIdea (central topic) found in data.xml")

        root_id = root_el.get("id")
        if not root_id:
            raise ValueError("floatingIdea has no id attribute")

        root_text = get_text(root_el)
        print(f"Root: {root_text}")

        graphics = collect_embedded_graphics(data_root, root_el, zip_names)
        n_float = len(graphics.get("floating_images") or [])
        cent = graphics.get("central_topic")
        has_central_asset = isinstance(cent, dict) and (
            cent.get("image_resource_id") or cent.get("central_idea_image_uuid")
        )
        if n_float or has_central_asset:
            print(f"Embedded graphics: central topic assets + {n_float} non-central raster cell(s)")

        tree = build_tree(data_root, elem_map, root_id, 0)
        if tree is None:
            raise RuntimeError("Failed to build tree from root id")

        payload = {
            "metadata": {
                "title": title,
                "author": author,
                "created": created,
            },
            "mindmap": tree,
            "embedded_graphics": graphics,
        }

        exported_paths: List[str] = []
        if export_image_files:
            exported_paths = export_embedded_image_files(zf, zip_names, output_dir, graphics)
            if exported_paths:
                print(f"Exported {len(exported_paths)} image file(s) to {output_dir / 'embedded_images'}")

        json_file = output_dir / "mindmap.json"
        json_file.write_text(
            json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
        print(f"JSON: {json_file}")

        text_file = output_dir / "mindmap.txt"
        header = f"{title}\nAuthor: {author}\nCreated: {created}\n\n"
        outline_body = "\n".join(build_text_lines(tree)) + "\n"
        asset_section = "\n".join(graphics_summary_lines(graphics))
        text_file.write_text(header + outline_body + "\n" + asset_section, encoding="utf-8")
        print(f"TEXT: {text_file}")

        opml_file = output_dir / "mindmap.opml"
        opml_body = build_opml(tree)
        assets_opml = build_opml_assets_section(graphics)
        body_inner = f"    {opml_body}"
        if assets_opml:
            body_inner += f"\n    {assets_opml}"
        opml_content = (
            '<?xml version="1.0" encoding="utf-8"?>\n'
            "<opml version=\"2.0\">\n"
            "  <head>\n"
            f"    <title>{html.escape(title)}</title>\n"
            f"    <ownerName>{html.escape(author)}</ownerName>\n"
            f"    <dateCreated>{html.escape(created)}</dateCreated>\n"
            "  </head>\n"
            "  <body>\n"
            f"{body_inner}\n"
            "  </body>\n"
            "</opml>\n"
        )
        opml_file.write_text(opml_content, encoding="utf-8")
        print(f"OPML: {opml_file}")

    print(f"\nDone! All formats exported to: {output_dir}")


def default_imx_path() -> Path:
    return Path(__file__).resolve().parent.parent / "tests" / "data" / "exemple.imx"


def default_output_dir() -> Path:
    return Path(__file__).resolve().parent.parent / "mindmap_exports"


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract .imx mind map to JSON, TXT, and OPML.")
    parser.add_argument(
        "--imx",
        type=Path,
        default=default_imx_path(),
        help="Path to the .imx file (ZIP archive). Default: tests/data/exemple.imx next to repo root.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=default_output_dir(),
        help="Directory for mindmap.json, mindmap.txt, mindmap.opml. Default: mindmap_exports/.",
    )
    parser.add_argument(
        "--export-embedded-images",
        action="store_true",
        help="Copy data/<id> raster blobs from the archive into output_dir/embedded_images/.",
    )
    args = parser.parse_args()

    imx_path: Path = args.imx
    output_dir: Path = args.output_dir

    if not imx_path.is_file():
        print(f"Error: IMX file not found: {imx_path}", file=sys.stderr)
        return 1

    try:
        extract_imx(imx_path, output_dir, args.export_embedded_images)
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
