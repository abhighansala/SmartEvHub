#ifndef USER_H
#define USER_H

#include <string>

// Simple User model for authentication and role-based access
// Created by: Aditya Singh (Team Lead - Database & Backend Foundation)
class User {
private:
    int id;
    std::string name;
    std::string email;
    std::string password;
    std::string role; // "admin", "operator", "user"
    std::string createdAt;

public:
    User() {
        id = 0;
        name = "";
        email = "";
        password = "";
        role = "user";
        createdAt = "";
    }

    User(int uId, std::string uName, std::string uEmail, std::string uPassword, std::string uRole, std::string uCreatedAt = "") {
        id = uId;
        name = uName;
        email = uEmail;
        password = uPassword;
        role = uRole;
        createdAt = uCreatedAt;
    }

    // Getters
    int getId() const { return id; }
    std::string getName() const { return name; }
    std::string getEmail() const { return email; }
    std::string getPassword() const { return password; }
    std::string getRole() const { return role; }
    std::string getCreatedAt() const { return createdAt; }

    // Setters
    void setId(int newId) { id = newId; }
    void setName(const std::string& newName) { name = newName; }
    void setEmail(const std::string& newEmail) { email = newEmail; }
    void setPassword(const std::string& newPassword) { password = newPassword; }
    void setRole(const std::string& newRole) { role = newRole; }
    void setCreatedAt(const std::string& newCreatedAt) { createdAt = newCreatedAt; }

    // Validation helper
    bool isValid() const {
        return !email.empty() && !name.empty();
    }
};

#endif
