"""
Auto-run on editor startup (Unreal executes any Content/Python/init_unreal.py when
the Python Editor Script Plugin is enabled). Registers the Keystone menu so artists
get the buttons with zero setup on their end.

To also auto-arm live import capture for everyone, uncomment the two lines below.
"""

import unreal

try:
    import keystone_menu
    keystone_menu.register_menu()

    # import keystone_source_sync
    # keystone_source_sync.register_import_hook()
except Exception as exc:  # never block editor startup
    unreal.log_warning(f"[keystone] init failed: {exc}")
