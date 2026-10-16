#pragma once

#include <string>

namespace evhub {

class User {
private:
    int id_;
    std::string name_;
    std::string email_;
    std::string passwordHash_;
    std::string role_; // "OPERATOR", "DRIVER", "ADMIN"
    std::string createdAt_;

public:
    User() : id_(0), role_("DRIVER") {}

    User(int id, std::string name, std::string email, std::string passwordHash, std::string role, std::string createdAt = "")
        : id_(id), name_(std::move(name)), email_(std::move(email)),
          passwordHash_(std::move(passwordHash)), role_(std::move(role)), createdAt_(std::move(createdAt)) {}

    // Getters with const correctness
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    const std::string& getEmail() const { return email_; }
    const std::string& getPasswordHash() const { return passwordHash_; }
    const std::string& getRole() const { return role_; }
    const std::string& getCreatedAt() const { return createdAt_; }

    void setId(int id) { id_ = id; }
    void setName(const std::string& name) { name_ = name; }
    void setRole(const std::string& role) { role_ = role; }
};

} // namespace evhub
