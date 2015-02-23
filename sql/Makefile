MODULE_big = jsonb_opx
DATA = jsonb_opx--1.0.sql jsonb_opx--1.1.sql jsonb_opx--1.0--1.1.sql
OBJS = jsonb_opx.o jsonb_utilsx.o
DOCS = README.md

EXTENSION = jsonb_opx
REGRESS = jsonb_opx

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
