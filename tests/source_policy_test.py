from pathlib import Path
import re
import sys


root = Path(sys.argv[1]).resolve()
meson = (root / "meson.build").read_text(encoding="utf-8")
match = re.search(r"cage_sources\s*=\s*\[(.*?)\n\]", meson, re.DOTALL)
assert match, "cage_sources block is missing"
source_names = set(re.findall(r"'([^']+\.[ch])'", match.group(1)))

required_sources = {
    "surface_control_protocol.c",
    "surface_controller.c",
    "surface_registry.c",
    "surface_token_hint.c",
    "surface_view_policy.c",
    "view.c",
}
assert required_sources <= source_names, "framework registry source is missing from Cage"
assert not {"poc_layout_controller.c", "poc_layout_socket.c"} & source_names, (
    "the legacy raw-width controller must remain test-only"
)

production_files = [root / name for name in sorted(source_names) if (root / name).is_file()]
production_files.extend(
    root / name
    for name in (
        "server.h",
        "surface_control_protocol.h",
        "surface_controller.h",
        "surface_registry.h",
        "surface_token_hint.h",
        "surface_view_policy.h",
        "view.h",
    )
)
production = "\n".join(path.read_text(encoding="utf-8") for path in production_files)
for forbidden in (
    "Linguum Workspace",
    "Linguum Browser Controls",
    "cg_poc_layout_classify_title",
    "view_update_poc_role",
    "CAGE_LINGUUM_LAYOUT_SOCKET",
):
    assert forbidden not in production, f"production Cage contains legacy identity: {forbidden}"

fixture = (root / "tests" / "poc_title_fixture.c").read_text(encoding="utf-8")
assert "Linguum Workspace" in fixture and "Linguum Browser Controls" in fixture
