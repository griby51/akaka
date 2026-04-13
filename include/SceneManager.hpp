#pragma once
#include "Scene.hpp"
#include <memory>
#include <stack>

class SceneManager{
public:
    void change(std::unique_ptr<Scene> scene);
    void push(std::unique_ptr<Scene> scene);
    void pop();
    Scene* current();
    bool isEmpty();
private:
    std::stack<std::unique_ptr<Scene>> mStack;
};
