SRC    = main.c
LIBS   = -lraylib -lgdi32 -lwinmm -lopengl32 -mwindows

all:
	gcc $(SRC) -o $(OUT) -I$(RAYLIB_DIR) -L$(RAYLIB_DIR) $(LIBS)
	@echo ">>> Finished Compiling Target: $(OUT)"
