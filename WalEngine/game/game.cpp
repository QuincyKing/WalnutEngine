#include "game.h"

void Game::init()
{
	albedo = Texture("pbr/plastic/albedo.png");
	normal = Texture("pbr/plastic/normal.png");
	metallic = Texture("pbr/plastic/metallic.png");
	roughness = Texture("pbr/plastic/roughness.png");
	ao = Texture("pbr/plastic/ao.png");

	Sphere::load();
	Cube::load();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	albedo.process();
	normal.process();
	metallic.process();
	roughness.process();
	ao.process();

	shader = Shader("pbr.vert", "pbr.frag");
	shader.use();

	shader.set_int("albedoMap", 0);
	shader.set_int("normalMap", 1);
	shader.set_int("metallicMap", 2);
	shader.set_int("roughnessMap", 3);
	//shader.set_int("aoMap", 4);
	albedo.bind(0);
	normal.bind(1);
	metallic.bind(2);
	roughness.bind(3);
	//ao.bind(4);

	sphere1.mTransform->set_pos(glm::vec3(1.0, 0.0, 0.0));
	sphere1.add_child(&sphere2);
	sphere2.add_child(&cube1);
	sphere2.mTransform->set_pos(glm::vec3(1.0, 1.0, 0.0));
	cube1.mTransform->set_pos(glm::vec3(-1.0, 0.0, 0.0));
	cube1.mTransform->set_rot(Quaternion(glm::vec3(1.0, 1.0, 0.0), glm::pi<float>() * 1 / 6));

	sphere1.set_shader(shader);
	sphere2.set_shader(shader);
	//cube1.set_shader(shader);

	root.add_child(&sphere1);
	root.add_child(&sphere4);
}

void Game::render(RenderEngine& renderer, Window& window)
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::vec2 curScreen;

	curScreen.x = 5 * float((Window::Inputs.get_mouse_x()) - Window::Inputs.get_win_size_x() / 2.0f) / float(Window::Inputs.get_win_size_x());
	curScreen.y = 5 * float(0 - (Window::Inputs.get_mouse_y()) - Window::Inputs.get_win_size_y() / 2.0f) / float(Window::Inputs.get_win_size_y());
	glm::vec3 lightPosition = glm::vec3(0.0f, 0.0f, 10.0f);
	glm::vec3 lightColor = glm::vec3(255.0f, 255.0f, 255.0f);
	shader.use();

	//shader->setMat4("projection", projection);
	glm::mat4 view = window.mCamera.get_view_projection();
	shader.set_mat4("vp", view);
	shader.set_vec3("camPos", *(window.mCamera.get_transform()->get_pos()));
	shader.set_vec3("lightPos", lightPosition + glm::vec3(curScreen, 0.0));
	shader.set_vec3("lightColor", lightColor);

	renderer.render(root, view);
	//cube1.render(shader2);
	//model.draw(shader);
}