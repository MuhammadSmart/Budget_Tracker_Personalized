#include "BudgetApp.hpp"

int main() {
    Storage storage("./data");
    BudgetApp app(std::move(storage));
    return app.run();
}
