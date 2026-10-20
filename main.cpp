#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>

// Структура параметрів та стану фізичної моделі (Варіант 8 + Базовий приклад)
struct PhysicsModel
{
    // Початкові параметри
    sf::Vector2f startPosition{ 100.f, 100.f };
    sf::Vector2f position{ 100.f, 100.f };
    float velocityY = 0.f;
    float velocityX = 60.f;          // Горизонтальний зсув для наочності траєкторії
    
    // Налаштовувані фізичні параметри
    float gravity = 980.f;           // Прискорення вільного падіння (пікс/с^2)
    float restitution = 0.8f;        // Коефіцієнт пружності відскоку (0..1)
    float impulseStrength = 450.f;   // Величина вертикального імпульсу (пікс/с)
    
    // Геометричні параметри сцени
    const float floorY = 550.f;
    const float ballRadius = 30.f;
    const float windowWidth = 800.f;

    // Статистика та керування станом симуляції
    int impulseCount = 0;
    float simulationTime = 0.f;
    bool isPaused = false;

    std::vector<sf::Vector2f> trajectory;
    const std::size_t maxTrajectoryPoints = 500;

    // Скидання моделі у початковий стан
    void reset()
    {
        position = startPosition;
        velocityY = 0.f;
        impulseCount = 0;
        simulationTime = 0.f;
        trajectory.clear();
    }

    // Надання керованого імпульсу вгору (Варіант 8)
    void applyUpwardImpulse()
    {
        if (!isPaused)
        {
            velocityY -= impulseStrength; // Вісь Y спрямована вниз, тому вгору — від'ємний напрямок
            impulseCount++;
        }
    }
};

// 1. Оброблення введення користувача (клавіатура та миша)
void handleInput(
    sf::RenderWindow& window,
    PhysicsModel& model,
    sf::Color& ballColor,
    const sf::RectangleShape& redBtn,
    const sf::RectangleShape& greenBtn,
    const sf::RectangleShape& blueBtn,
    const sf::RectangleShape& whiteBtn)
{
    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }

        // Обробка натискання клавіш
        if (event->is<sf::Event::KeyPressed>())
        {
            const auto* key = event->getIf<sf::Event::KeyPressed>();

            // Зміна прискорення вільного падіння (Up / Down)
            if (key->code == sf::Keyboard::Key::Up)
                model.gravity += 50.f;
            if (key->code == sf::Keyboard::Key::Down)
                model.gravity -= 50.f;

            // Зміна коефіцієнта пружності (Right / Left)
            if (key->code == sf::Keyboard::Key::Right)
                model.restitution += 0.05f;
            if (key->code == sf::Keyboard::Key::Left)
                model.restitution -= 0.05f;

            // Зміна сили керованого імпульсу (W / S)
            if (key->code == sf::Keyboard::Key::W)
                model.impulseStrength += 50.f;
            if (key->code == sf::Keyboard::Key::S)
                model.impulseStrength -= 50.f;

            // Надання імпульсу вгору за натисканням Space (Варіант 8)
            if (key->code == sf::Keyboard::Key::Space)
            {
                model.applyUpwardImpulse();
            }

            // Пауза / Продовження моделювання (P)
            if (key->code == sf::Keyboard::Key::P)
            {
                model.isPaused = !model.isPaused;
            }

            // Повторний запуск (скидання у початкове положення) (R)
            if (key->code == sf::Keyboard::Key::R)
            {
                model.reset();
            }

            // Обмеження значень параметрів у допустимих межах
            model.gravity = std::clamp(model.gravity, 100.f, 2000.f);
            model.restitution = std::clamp(model.restitution, 0.f, 1.f);
            model.impulseStrength = std::clamp(model.impulseStrength, 100.f, 1200.f);
        }

        // Обробка натискання кнопок миші для зміни кольору м'яча (Етап 8)
        if (event->is<sf::Event::MouseButtonPressed>())
        {
            const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>();
            sf::Vector2f mousePos(static_cast<float>(mouse->position.x),
                                  static_cast<float>(mouse->position.y));

            if (redBtn.getGlobalBounds().contains(mousePos))
                ballColor = sf::Color::Red;
            else if (greenBtn.getGlobalBounds().contains(mousePos))
                ballColor = sf::Color::Green;
            else if (blueBtn.getGlobalBounds().contains(mousePos))
                ballColor = sf::Color::Blue;
            else if (whiteBtn.getGlobalBounds().contains(mousePos))
                ballColor = sf::Color::White;
        }
    }
}

// 2. Оновлення фізичної моделі з урахуванням часового кроку dt
void updatePhysics(PhysicsModel& model, float dt)
{
    if (model.isPaused)
        return;

    // Обмеження надто великого кроку часу для збереження чисельної стійкості
    dt = std::min(dt, 0.05f);
    model.simulationTime += dt;

    // Чисельне інтегрування методом Ейлера-Кромера
    model.velocityY += model.gravity * dt;
    model.position.y += model.velocityY * dt;
    model.position.x += model.velocityX * dt;

    // Відбиття від бічних стінок вікна для неперервності візуалізації траєкторії
    if (model.position.x + 2.f * model.ballRadius >= model.windowWidth)
    {
        model.position.x = model.windowWidth - 2.f * model.ballRadius;
        model.velocityX = -model.velocityX;
    }
    else if (model.position.x <= 0.f)
    {
        model.position.x = 0.f;
        model.velocityX = -model.velocityX;
    }

    // Перевірка зіткнення з підлогою з урахуванням діаметра м'яча (Етапи 3-4)
    if (model.position.y + 2.f * model.ballRadius >= model.floorY)
    {
        model.position.y = model.floorY - 2.f * model.ballRadius;
        model.velocityY = -model.velocityY * model.restitution;

        if (std::abs(model.velocityY) < 15.f)
            model.velocityY = 0.f;
    }

    // Обмеження верхньої межі вікна
    if (model.position.y < 0.f)
    {
        model.position.y = 0.f;
        model.velocityY = -model.velocityY * 0.5f;
    }

    // Збереження точок у траєкторію (Етап 7)
    sf::Vector2f centerPoint{
        model.position.x + model.ballRadius,
        model.position.y + model.ballRadius
    };
    model.trajectory.push_back(centerPoint);

    if (model.trajectory.size() > model.maxTrajectoryPoints)
    {
        model.trajectory.erase(model.trajectory.begin());
    }
}

// 3. Відображення графічної сцени
void drawScene(
    sf::RenderWindow& window,
    const PhysicsModel& model,
    sf::CircleShape& ball,
    const sf::RectangleShape& floor,
    sf::Text& infoText,
    const sf::RectangleShape& redBtn,
    const sf::RectangleShape& greenBtn,
    const sf::RectangleShape& blueBtn,
    const sf::RectangleShape& whiteBtn)
{
    window.clear(sf::Color::Black);

    ball.setPosition(model.position);

    sf::VertexArray trajectoryLine(sf::PrimitiveType::LineStrip);
    for (const auto& point : model.trajectory)
    {
        trajectoryLine.append(sf::Vertex{ point, sf::Color(150, 150, 150) });
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "State: " << (model.isPaused ? "PAUSED" : "RUNNING")
        << " [P: Pause, R: Reset]\n"
        << "Gravity (Up/Down): " << model.gravity << " px/s^2\n"
        << "Restitution (Left/Right): " << model.restitution << "\n"
        << "Impulse Power (W/S): " << model.impulseStrength << " px/s\n"
        << "Velocity Y: " << model.velocityY << " px/s\n"
        << "Height above floor: " << (model.floorY - (model.position.y + 2.f * model.ballRadius)) << " px\n"
        << "Impulses Used [Space]: " << model.impulseCount << "\n"
        << "Sim Time: " << model.simulationTime << " s";

    infoText.setString(oss.str());

    window.draw(floor);
    window.draw(trajectoryLine);
    window.draw(ball);
    window.draw(infoText);

    window.draw(redBtn);
    window.draw(greenBtn);
    window.draw(blueBtn);
    window.draw(whiteBtn);

    window.display();
}

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({ 800, 600 }),
        "SFML Physics Demo - Variant 8 (Controlled Impulse)"
    );
    window.setFramerateLimit(120);

    PhysicsModel model;

    sf::CircleShape ball(model.ballRadius);
    sf::Color ballColor = sf::Color::White;
    ball.setFillColor(ballColor);
    ball.setPosition(model.position);

    sf::RectangleShape floor({ 800.f, 50.f });
    floor.setFillColor(sf::Color(100, 100, 100));
    floor.setPosition({ 0.f, model.floorY });

    sf::RectangleShape redButton({ 40.f, 40.f });
    redButton.setPosition({ 550.f, 20.f });
    redButton.setFillColor(sf::Color::Red);

    sf::RectangleShape greenButton({ 40.f, 40.f });
    greenButton.setPosition({ 600.f, 20.f });
    greenButton.setFillColor(sf::Color::Green);

    sf::RectangleShape blueButton({ 40.f, 40.f });
    blueButton.setPosition({ 650.f, 20.f });
    blueButton.setFillColor(sf::Color::Blue);

    sf::RectangleShape whiteButton({ 40.f, 40.f });
    whiteButton.setPosition({ 700.f, 20.f });
    whiteButton.setFillColor(sf::Color::White);

    sf::Font font;
    if (!font.openFromFile("../resources/fonts/arial.ttf"))
    {
        if (!font.openFromFile("C:/Windows/Fonts/arial.ttf"))
        {
            return 1;
        }
    }

    sf::Text infoText(font, "", 18);
    infoText.setFillColor(sf::Color::White);
    infoText.setPosition({ 20.f, 15.f });

    sf::Clock clock;

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();

        handleInput(window, model, ballColor, redButton, greenButton, blueButton, whiteButton);
        ball.setFillColor(ballColor);

        updatePhysics(model, dt);

        drawScene(window, model, ball, floor, infoText, redButton, greenButton, blueButton, whiteButton);
    }

    return 0;
}