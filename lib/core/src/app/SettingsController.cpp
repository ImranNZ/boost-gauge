#include "app/SettingsController.h"

namespace gauge {

void SettingsController::tick() {
  if (!menu_.handle(input_.poll())) return;
  store_.save(menu_.setup());
  target_.apply(menu_.setup());
}

}  // namespace gauge
