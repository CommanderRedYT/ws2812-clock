const configExplorer = document.getElementById("configExplorer"); // table

const parseType = (config) => {
    if (config.value && config.value.value && config.value.values) {
        return 'enum';
    }

    return 'text';
};

const parseValue = (config, enumWithPrefix = false) => {
    if (config.value && config.value.value && config.value.values) {
        if (enumWithPrefix) {
            return `${config.type}::${config.value.value}`;
        }
        return config.value.value;
    }
    return JSON.stringify(config.value);
};

const inputFromType = (config, type) => {
    switch (type) {
        case "enum":
            const select = document.createElement("select");
            const values = config.value.values;

            values.forEach((value) => {
                const option = document.createElement("option");
                option.setAttribute("value", value);
                option.setAttribute("data-type", config.type);
                option.setAttribute("data-value", value);
                if (config.value.value === value)
                    option.setAttribute("selected", "selected");
                option.innerText = value;
                select.appendChild(option);
            });

            select.setAttribute("value", config.value.value);

            return select;
        case "text":
            const text = document.createElement("input");
            text.setAttribute("type", "text");
            text.setAttribute("value", JSON.stringify(config.value));
            return text;
        default:
            const element = document.createElement("p");
            console.warn("unknown type", type);
            element.innerText = "unknown type";
            return element;
    }
};

const createConfigSetForm = (key, config) => {
    const form = document.createElement("form");
    const input = inputFromType(config, parseType(config));
    const button = document.createElement("button");

    input.setAttribute("name", key);
    input.setAttribute("data-type", config.type);

    button.setAttribute("type", "submit");
    button.innerText = "Set";

    form.appendChild(input);
    form.appendChild(button);

    return form;
};

const createOrUpdateConfigRow = (key, config) => {
    let row = null;
    let name = null;
    let value = null;
    let type = null;
    let result = null;
    let set = null;

    const existingRow = configExplorer.querySelector(
        `[data-key="${key}"]`
    );

    if (existingRow) {
        row = existingRow;
        name = row.querySelector("td:nth-child(1)");
        value = row.querySelector("td:nth-child(2)");
        type = row.querySelector("td:nth-child(3)");
        set = row.querySelector("td:nth-child(4) form");
        result = set.querySelector("[data-result]");
    } else {
        row = document.createElement("tr");
        name = document.createElement("td");
        value = document.createElement("td");
        type = document.createElement("td");
        set = document.createElement("td");
        set.appendChild(createConfigSetForm(key, config));
        result = document.createElement("p");

        set.addEventListener("submit", (e) => {
            e.preventDefault();
            const inputOrSelect = e.target.querySelector("input, select");
            const resultElement = document.querySelector(
                `[data-result="result"][data-key="${key}"]`
            );

            try {
                const key = inputOrSelect.getAttribute("name");
                const elementValue = inputOrSelect.value;
                const isSelect = inputOrSelect.tagName === "SELECT";

                const value = JSON.parse(isSelect ? `"${elementValue}"` : elementValue);

                window.clockApi.setConfig({[key]: value}).then((result) => {
                    resultElement.innerText = JSON.stringify(result);
                });
            } catch (e) {
                console.error('hatschi', e, inputOrSelect);
                resultElement.innerText = e.message;
            }
        });

        result.setAttribute("data-result", "result");
        result.setAttribute("data-key", key);
    }

    // set text content
    if (name.innerText !== key)
        name.innerText = key;

    const parsedValue = parseValue(config);
    if (value.innerText !== parsedValue)
        value.innerText = parsedValue;

    if (type.innerText !== config.type)
        type.innerText = config.type;

    row.setAttribute("data-key", key);
    row.setAttribute("data-type", config.type);
    row.setAttribute("data-touched", config.touched);
    row.setAttribute("data-value", JSON.stringify(config.value));

    if (!existingRow) {
        row.appendChild(name);
        row.appendChild(value);
        row.appendChild(type);
        row.appendChild(set);
        set.appendChild(result);

        return row;
    }

    return null;
};


export const handleConfigChange = (config) => {
    const tbody = configExplorer.querySelector("tbody");

    for (const [key, value] of Object.entries(config)) {
        const row = createOrUpdateConfigRow(key, value);
        if (row)
            tbody.appendChild(row);
    }
};

export const handleClearConfig = () => {
    const tbody = configExplorer.querySelector("tbody");
    tbody.innerHTML = "";
};
