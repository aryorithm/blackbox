/**
 * @file rule_engine.h
 * @brief Deterministic Rule Matcher (Sigma-style).
 * 
 * Complements the AI Engine.
 * - AI finds "Unknown Unknowns" (Anomalies).
 * - Rule Engine finds "Known Knowns" (Signatures).
 */

#ifndef BLACKBOX_ANALYSIS_RULE_ENGINE_H
#define BLACKBOX_ANALYSIS_RULE_ENGINE_H

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include "blackbox/parser/parser_engine.h" // For ParsedLog

namespace blackbox::analysis {

    enum class RuleAction {
        ALERT,
        DROP,
        TAG
    };

    struct Rule {
        std::string name;
        std::string description;
        RuleAction action;
        
        // Conditions (Simplified for MVP)
        // In production, this would be an Expression Tree (AST)
        std::string field_target; // e.g., "service", "message", "host"
        std::string pattern;      // e.g., "sshd", "DROP", "192.168.1.100"
        bool is_regex;
    };

    class RuleEngine {
    public:
        RuleEngine();
        ~RuleEngine() = default;

        /**
         * @brief Load rules from a YAML/JSON configuration file.
         * @param config_path Path to rules.yaml
         */
        void load_rules(const std::string& config_path);

        /**
         * @brief Evaluate a log against all active rules.
         * 
         * @param log The parsed log structure
         * @return std::optional<std::string> The name of the matched rule, or nullopt.
         */
        std::optional<std::string> evaluate(const parser::ParsedLog& log);

    private:
        std::vector<Rule> rules_;
        
        // Helper to check string containment
        bool match_condition(std::string_view value, const Rule& rule);
    };

} // namespace blackbox::analysis

#endif // BLACKBOX_ANALYSIS_RULE_ENGINE_H