#pragma once

#include <string>

namespace finderx::ui {

std::wstring dragFeedbackText(bool canMove, std::size_t itemCount, const std::wstring& destinationName);

}
