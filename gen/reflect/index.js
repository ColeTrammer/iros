const fs = require("fs");
const path = require("path");

const result = { input: process.argv[2], output: process.argv[3] };

const contents = fs.readFileSync(result["input"], "utf8");
const relevant = contents.replace(/[\s\S]*\/\/ iros reflect begin([\s\S]*)\/\/ iros reflect end[\s\S]*/gm, "$1");

const removeMultilineImplementation = (s) => {
    let fixed = "";
    let buffer = "";
    let count = 0;
    for (const c of s) {
        if (c == "{") {
            buffer = "";
            count++;
        }
        if (c == "}") {
            if (--count === 0) {
                fixed += buffer;
            }
        }
        if (count > 0) {
            if (c !== "\n") {
                buffer += c;
            }
        } else {
            fixed += c;
        }
    }
    return fixed;
};

const fixTemplates = (s) => {
    return s.replaceAll(/template(.*)\n(.*)/gm, "template$1$2");
};

const lines = fixTemplates(removeMultilineImplementation(relevant))
    .split("\n")
    .map((s) => s.trim())
    .filter((s) => s.length > 0);

const processParams = (p) => {
    const items = p
        .substring(1, p.length - 1)
        .split(",")
        .filter((x) => x.length !== 0);
    return items.map((s) => {
        const names = s.split(" ").filter((x) => x.length !== 0);
        const type = names.slice(0, names.length - 1).join(" ");
        const name = names[names.length - 1];
        return { type, name };
    });
};

const methods = lines
    .map((line) => {
        return line.replaceAll("virtual", "").replaceAll(" = 0;", "").replaceAll(/{.*}/g, "").replaceAll(";", "").trim();
    })
    .map((line) => {
        return line.split(" ");
    })
    .map((line) => {
        const index = line.findIndex((s) => s.includes("("));
        return [line.slice(0, index).join(" "), line[index], ...line.slice(index + 1)];
    })
    .map((line) => {
        if (line[line.length - 1] === "const") {
            return [line[0], line.slice(1, line.length - 1).join(" "), line[line.length - 1]];
        }
        return [line[0], line.slice(1).join(" "), ""];
    })
    .map((line) => {
        return [line[0], line[1].split("(")[0], "(" + line[1].split("(")[1], line[2]];
    })
    .map((line) => {
        return { returnType: line[0], name: line[1], params: processParams(line[2]), modifiers: line[3] };
    });

const forwardMethod = (method) => {
    const processArg = ({ type, name }) => {
        if (type.includes("...")) {
            return `forward<${type.replaceAll("&&", "").replaceAll("...", "")}>(${name})...`;
        }
        return `forward<${type}>(${name})`;
    };

    const processTemplate = (s) => {
        if (!s.includes("template")) {
            return "";
        }
        const args = processParams(s.replaceAll(/.*template(<.*>  ).*/g, "$1").trim());
        return `<${args.map(({ type, name }) => (type.includes("...") ? `${name}...` : name)).join(", ")}>`;
    };

    const hasTemplate = method.returnType.includes("template");
    return `    ${hasTemplate ? "" : "template<typename = void> "}${method.returnType} ${method.name}(${method.params
        .map(({ type, name }) => `${type} ${name}`)
        .join(", ")}) ${method.modifiers} { return __o_.${method.name}${processTemplate(method.returnType)}(${method.params
        .map(processArg)
        .join(", ")}); }`;
};

const forward = (methods) => {
    return `#pragma once

#define ${result["output"].replaceAll("/", "_").replaceAll(".h", "").toUpperCase()}_FORWARD(__o_) \\
public: \\
${methods.map(forwardMethod).join(" \\\n")} \\
\\
private:
`;
};

try {
    fs.mkdirSync(path.dirname(result["output"], { recursive: true }));
} catch {}
fs.writeFileSync(result["output"], forward(methods));
