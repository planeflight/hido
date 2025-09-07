#ifndef CAR_HPP
#define CAR_HPP

#include <raylib.h>

struct Car {
    explicit Car(const Rectangle &rect);
    ~Car();

    void input(float dt);
    void render();
    void update(float dt);

    Rectangle rect;
    float direction = 0.0f;
    Texture texture;
    float speed = 0.0f;

    bool forward = false;

    constexpr static float SPEED = 500.0f;
    constexpr static float ACCELERATION = 900.0f;
};

#endif // CAR_HPP
