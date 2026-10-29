-- -------------------------------------------------------------
-- Intelligent Civic Grievance Detection, Routing and Tracking System
-- MySQL 8.0 Database Schema
-- -------------------------------------------------------------

CREATE DATABASE IF NOT EXISTS civic_grievance CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE civic_grievance;

-- -------------------------------------------------------------
-- 1. Departments Table
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS departments (
    department_id   INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    name            VARCHAR(64) NOT NULL UNIQUE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Initial Department Records matching the project specifications
INSERT IGNORE INTO departments (department_id, name) VALUES
    (1, 'Road'),
    (2, 'Sanitation'),
    (3, 'Electrical'),
    (4, 'Water'),
    (5, 'Drainage'),
    (6, 'General');

-- -------------------------------------------------------------
-- 2. Users Table (Super Admin & Department Admin)
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS users (
    user_id         BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    username        VARCHAR(64) NOT NULL UNIQUE,
    password_hash   VARCHAR(255) NOT NULL,
    role            ENUM('SuperAdmin', 'DeptAdmin') NOT NULL,
    department_id   INT UNSIGNED NULL,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_users_department
        FOREIGN KEY (department_id) REFERENCES departments(department_id)
        ON UPDATE CASCADE ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE INDEX idx_users_role ON users(role);
CREATE INDEX idx_users_department ON users(department_id);

-- -------------------------------------------------------------
-- 3. Complaints Table
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS complaints (
    complaint_id    BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    citizen_name    VARCHAR(128) NOT NULL,
    citizen_contact VARCHAR(128) NULL,
    category        VARCHAR(64) NOT NULL,
    description     TEXT NOT NULL,
    location        VARCHAR(256) NOT NULL,
    department_id   INT UNSIGNED NOT NULL,
    priority        ENUM('Low', 'Medium', 'High') NOT NULL,
    status          VARCHAR(32) NOT NULL DEFAULT 'Registered',
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    CONSTRAINT fk_complaints_department
        FOREIGN KEY (department_id) REFERENCES departments(department_id)
        ON UPDATE CASCADE ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE INDEX idx_complaints_department ON complaints(department_id);
CREATE INDEX idx_complaints_status ON complaints(status);
CREATE INDEX idx_complaints_priority ON complaints(priority);
CREATE INDEX idx_complaints_category ON complaints(category);

-- -------------------------------------------------------------
-- 4. Status History Table (Audit Trail)
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS status_history (
    history_id      BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    complaint_id    BIGINT UNSIGNED NOT NULL,
    previous_status VARCHAR(32) NOT NULL,
    new_status      VARCHAR(32) NOT NULL,
    changed_by      BIGINT UNSIGNED NOT NULL,
    notes           VARCHAR(255) NULL,
    changed_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_history_complaint
        FOREIGN KEY (complaint_id) REFERENCES complaints(complaint_id)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT fk_history_user
        FOREIGN KEY (changed_by) REFERENCES users(user_id)
        ON UPDATE CASCADE ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE INDEX idx_history_complaint_id ON status_history(complaint_id);
CREATE INDEX idx_history_changed_by ON status_history(changed_by);
