MODULES = jsonb_opx
DATA_built = jsonb_opx.sql
OBJS = jsonb_opx.o
DOCS = README.md

REGRESS = jsonb_opx

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
