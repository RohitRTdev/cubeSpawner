#include <iostream>
#include <vector>
#include <array>
#include <Windows.h>


struct console {
	int rows;
	int cols;
	int m_color;

	HANDLE screenHandle;
	console() {
		m_color = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
		screenHandle = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

		SetConsoleActiveScreenBuffer(screenHandle);
		CONSOLE_SCREEN_BUFFER_INFO screen_info{};
		GetConsoleScreenBufferInfo(screenHandle, &screen_info);

		cols = screen_info.dwSize.X;
		rows = screen_info.dwSize.Y;

		buf.resize(rows * cols);
		clear();
		write();
	}

	void clear() {
		std::fill(buf.begin(), buf.end(), CHAR_INFO{ L' ', 0 });
	}

	void write() {
		COORD start = { 0, 0 };
		
		SMALL_RECT rect{ 0, 0, cols - 1, rows - 1 };
		COORD bufSize{ cols, rows };
		WriteConsoleOutput(screenHandle, buf.data(), bufSize, start, &rect);
	}

	void fill_char(int x, int y, WCHAR fill = L'*') {
		if (x >= 0 && y >= 0 && x < cols && y < rows) {
			buf[y*cols + x].Char.UnicodeChar = fill;
			buf[y*cols + x].Attributes = m_color;
		}
	}

	void set_color(int color) {
		m_color = color;
	}

	void update_screen_size() {
		CONSOLE_SCREEN_BUFFER_INFO con_info{};
		GetConsoleScreenBufferInfo(screenHandle, &con_info);
		int new_rows = con_info.dwSize.Y;
		int new_cols = con_info.dwSize.X;
		
		if (rows != new_rows || cols != new_cols) {
			buf.resize(new_rows * new_cols);
		}
		rows = new_rows;
		cols = new_cols;
	}

private:
	std::vector<CHAR_INFO> buf;
};

struct vertex {
	double x;
	double y;
	double z;
};

struct coord {
	int x;
	int y;
};

struct cube {
	std::array<vertex, 8> vertices;
	console& con;
	static const bool projection = true;


	cube(vertex vert[], size_t N, console& active_con) : con(active_con){
		for (int i = 0; i < N; i++)
			vertices[i] = vert[i];
	}

	static void set_scale(const double sc) {
		scale = sc;
	}

	void shift(const double step_size, int direction) {
		for (auto& vert : vertices) {
			switch (direction) {
				case 0: {
					//Left
					vert.x -= step_size; break;
				}
				case 1: {
					//Up
					vert.y += step_size; break;
				}
				case 2: {
					//Right
					vert.x += step_size; break;
				}
				case 3: {
					//Down
					vert.y -= step_size; break;
				}
				case 4: {
					//Forward
					vert.z += step_size; break;
				}
				case 5: {
					//Backward
					vert.z -= step_size; break;
				}
			}
		}
	}

	void rotate_about_z(double angle) {
		for (auto& vert : vertices) {
			double new_x = vert.x*cos(angle) + vert.y * sin(angle);
			double new_y = vert.y * cos(angle) - vert.x * sin(angle);
			vert.x = new_x;
			vert.y = new_y;
		}
	}

	void rotate_about_x(double angle) {
		for (auto& vert : vertices) {
			double new_y = vert.y * cos(angle) + vert.z * sin(angle);
			double new_z = vert.z * cos(angle) - vert.y * sin(angle);
			vert.y = new_y;
			vert.z = new_z;
		}
	}
	
	void rotate_about_y(double angle) {
		for (auto& vert : vertices) {
			double new_x = vert.x * cos(angle) - vert.z * sin(angle);
			double new_z = vert.x * sin(angle) + vert.z * cos(angle);
			vert.x = new_x;
			vert.z = new_z;
		}
	}

	void draw_line(int x1, int y1, int x2, int y2) {
		int x_start, y_start, x_end, y_end;
		
		if (x1 == x2 && y1 == y2) {
			con.fill_char(x1, y1);
			return;
		}

		if (x1 < x2) {
			x_start = x1;
			y_start = y1;
			x_end = x2;
			y_end = y2;
		}
		else if (x1 == x2) {
			x_start = x_end = x1;
			y_start = min(y1, y2);
			y_end = max(y1, y2);
		}
		else {
			x_start = x2;
			y_start = y2;
			x_end = x1;
			y_end = y1;
		}

		bool is_inf = false;
		double slope;
		if (x_start == x_end) {
			is_inf = true;
		}
		else {
			slope = (y_start - (double)y_end) / (x_end - (double)x_start);
		}

		if (is_inf) {
			for (int y = y_start; y < y_end; y++) {
				con.fill_char(x_start, y);
			}
		}
		else if (slope < 1 && slope > -1) {
			for (int x = x_start; x < x_end; x++) {
				int y_calc = slope * (x - x_start) - y_start;
				con.fill_char(x, -y_calc);
			}
		}
		else {
			for (int y = min(y_start, y_end); y < max(y_end, y_start); y++) {
				int x_calc = x_start + (y_start - y) / slope;
				con.fill_char(x_calc, y);
			}
		}
	}

	static void zoom(const double step_size, int direction) {
		if (direction == 0) {
			//Zoom in
			scale -= step_size;
		}
		else {
			scale += step_size;
		}
		scale = min(max(scale, 1), 1000);
	}

	void perspective_projection(double& x, double& y, double& z) {
		x = x / (2 - z);
		y = y / (2 - z);
	}

	void draw() {
		//Orthogonal projection
		for (int i = 0; i < screen_coord.size(); i++) {
			double x_vert = vertices[i].x;
			double y_vert = vertices[i].y;
			double z_vert = vertices[i].z;

			//Normalize to [-1,1] space
			x_vert /= scale;
			y_vert /= scale;
			z_vert /= scale;

			//Otherwise defaults to orthogonal projection
			if(projection)
				perspective_projection(x_vert, y_vert, z_vert);
			
			//Switch to 4th quadrant
			x_vert += 0.5;
			y_vert -= 0.5;

			//Switch to screen coordinates
			screen_coord[i].x = x_vert * con.cols;
			screen_coord[i].y = -y_vert * con.rows;
		}

		draw_line(screen_coord[0].x, screen_coord[0].y, screen_coord[1].x, screen_coord[1].y);
		draw_line(screen_coord[0].x, screen_coord[0].y, screen_coord[3].x, screen_coord[3].y);
		draw_line(screen_coord[0].x, screen_coord[0].y, screen_coord[4].x, screen_coord[4].y);
		draw_line(screen_coord[1].x, screen_coord[1].y, screen_coord[2].x, screen_coord[2].y);
		draw_line(screen_coord[1].x, screen_coord[1].y, screen_coord[5].x, screen_coord[5].y);
		draw_line(screen_coord[2].x, screen_coord[2].y, screen_coord[6].x, screen_coord[6].y);
		draw_line(screen_coord[2].x, screen_coord[2].y, screen_coord[3].x, screen_coord[3].y);
		draw_line(screen_coord[3].x, screen_coord[3].y, screen_coord[7].x, screen_coord[7].y);
		draw_line(screen_coord[4].x, screen_coord[4].y, screen_coord[5].x, screen_coord[5].y);
		draw_line(screen_coord[4].x, screen_coord[4].y, screen_coord[7].x, screen_coord[7].y);
		draw_line(screen_coord[5].x, screen_coord[5].y, screen_coord[6].x, screen_coord[6].y);
		draw_line(screen_coord[6].x, screen_coord[6].y, screen_coord[7].x, screen_coord[7].y);

	}


private:
	std::array<coord, 8> screen_coord;
	static double scale;
};

double cube::scale = 0;

struct Scene {
	std::vector<cube> cubes;
	std::array<vertex, 8> starting_mesh;
	int active_cube;
	WORD color_palette[6];

	console& con;
	Scene(vertex vert[], console& active_con) : con(active_con), active_cube(-1) {
		for (int i = 0; i < 8; i++) {
			starting_mesh[i] = vert[i];
		}

		color_palette[0] = FOREGROUND_RED;
		color_palette[1] = FOREGROUND_BLUE;
		color_palette[2] = FOREGROUND_GREEN;
		color_palette[3] = FOREGROUND_BLUE | FOREGROUND_GREEN;
		color_palette[4] = FOREGROUND_BLUE | FOREGROUND_RED;
		color_palette[5] = FOREGROUND_RED | FOREGROUND_GREEN;
	
		cube::set_scale(2);
	}

	void render() {
		int color_idx = 0;
		con.clear();
		int pos = 0;
		for (auto& obj: cubes) {
			if (pos == active_cube) {
				con.set_color(color_palette[pos % 6]);
			}
			else {
				con.set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			}
			obj.draw();
			pos++;
		}
		con.write();
	}

	void add_cube() {
		cubes.push_back(cube(starting_mesh.data(), starting_mesh.size(), con));
		active_cube = cubes.size() - 1;
	}

	cube& fetch_active_cube() {
		return cubes[active_cube];
	}

	void toggle_cube() {
		active_cube = (active_cube + 1) % cubes.size();
	}

	void zoom(const double step_size, int direction) {
		cube::zoom(step_size, direction);
	}
};

bool is_key_pressed(int vk) {
	return GetKeyState(vk) & 0x8000;
}

int main() {
	console con;

	vertex vertices[] = { { -0.5,-0.5,0.5 }, { -0.5,0.5,0.5 }, { 0.5,0.5,0.5 }, { 0.5,-0.5,0.5 },
		{ -0.5,-0.5,-0.5 }, { -0.5,0.5,-0.5 }, { 0.5,0.5,-0.5 }, { 0.5,-0.5, -0.5 } };


	Scene scene(vertices, con);
	scene.add_cube();
	const int frame_time_ms = 20;
	const int frame_rate = 1000 / frame_time_ms;
	const int radians_per_second = 1;
	const double radians_per_frame = (double)radians_per_second / frame_rate;
	const double step_size = (double)1 / frame_rate;
	const double zoom_step = (double)1 / frame_rate;
	bool t_key_pressed = false, m_key_pressed = false;
	while (1) {
		scene.render();

		char key;
		std::cin >> key;
		if (is_key_pressed(VK_LEFT)) {
			scene.fetch_active_cube().shift(step_size, 0);
		}
		else if (is_key_pressed(VK_UP)) {
			scene.fetch_active_cube().shift(step_size, 1);
		}
		else if (is_key_pressed(VK_RIGHT)) {
			scene.fetch_active_cube().shift(step_size, 2);
		}
		else if (is_key_pressed(VK_DOWN)) {
			scene.fetch_active_cube().shift(step_size, 3);
		}
		else if (is_key_pressed('z') || is_key_pressed('Z')) {
			scene.fetch_active_cube().shift(step_size, 4);
		}
		else if (is_key_pressed('x') || is_key_pressed('X')) {
			scene.fetch_active_cube().shift(step_size, 5);
		}
		else if (is_key_pressed('a') || is_key_pressed('A')) {
			scene.fetch_active_cube().rotate_about_y(radians_per_frame);
		}
		else if (is_key_pressed('d') || is_key_pressed('D')) {
			scene.fetch_active_cube().rotate_about_y(-radians_per_frame);
		}
		else if (is_key_pressed('w') || is_key_pressed('W')) {
			scene.fetch_active_cube().rotate_about_x(radians_per_frame);
		}
		else if (is_key_pressed('s') || is_key_pressed('S')) {
			scene.fetch_active_cube().rotate_about_x(-radians_per_frame);
		}
		else if (is_key_pressed('q') || is_key_pressed('Q')) {
			scene.fetch_active_cube().rotate_about_z(-radians_per_frame);
		}
		else if (is_key_pressed('e') || is_key_pressed('E')) {
			scene.fetch_active_cube().rotate_about_z(radians_per_frame);
		}
		else if (is_key_pressed('g') || is_key_pressed('G')) {
			scene.zoom(zoom_step, 0);
		}
		else if (is_key_pressed('f') || is_key_pressed('F')) {
			scene.zoom(zoom_step, 1);
		}
		else if (is_key_pressed('m') || is_key_pressed('M')) { //Spawns a new cube
			m_key_pressed = true;
		}
		else if (is_key_pressed('t') || is_key_pressed('T')) { //Selects a different cube(so that u can move it, rotate it etc)
			t_key_pressed = true;
		}
		else {
			if (m_key_pressed) {
				m_key_pressed = false;
				scene.add_cube();
			}
			else if (t_key_pressed) {
				t_key_pressed = false;
				scene.toggle_cube();
			}
		}

		con.update_screen_size();
		Sleep(frame_time_ms);
	}

}