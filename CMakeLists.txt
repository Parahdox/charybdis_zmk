# Right half (split central) only: re-pair automatically if the left half has
# lost its bond. See src/split_rebond.c.
if(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  zephyr_library()
  zephyr_library_sources(src/split_rebond.c)
endif()
