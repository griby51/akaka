#include "SceneManager.hpp"

void SceneManager::change(std::unique_ptr<Scene> scene){
    if(!mStack.empty()) mStack.pop();
    mStack.push(std::move(scene));
}

void SceneManager::push(std::unique_ptr<Scene> scene){
    mStack.push(std::move(scene));
}

void SceneManager::pop(){
    if(!mStack.empty()){
        mStack.pop();
    }
}

Scene* SceneManager::current(){
    return mStack.empty() ? nullptr : mStack.top().get();
}

bool SceneManager::isEmpty(){
    return mStack.empty();
}
