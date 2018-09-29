import Foundation

let generations = 100

struct PatternString {
    var str : String
    
    init(_ string: String, padding: Int? = nil) {
        let paddingStr = String(repeating: ".", count: padding ?? 0)
        str = paddingStr + string + paddingStr
    }
    
    // offset from the start to the first marked position
    lazy var offset = {str.distance(from: str.startIndex, to: str.index(of: "#") ?? str.endIndex)}()
    
    // string without any leading or trailing blanks
    lazy var trimmed = {str.trimmingCharacters(in: CharacterSet(charactersIn: "."))}()
    
    // Generate next row according to rules
    func nextRow() -> PatternString {
        var newRow = Array(repeating: ".", count: str.count)
        let oldRow = Array(str)
        for i in 2..<(str.count - 2) {
            let marks = oldRow[(i - 2)...(i + 2)].reduce(0, {$0 + ($1 == "#" ? 1 : 0)})
            newRow[i] = (marks == 3 || marks == 5 || (oldRow[i] == "." && marks == 2) ? "#" : ".")
        }
        return PatternString(newRow.joined())
    }
}

// Process one pattern, returning its type
func process(_ pattern: String) -> String {
    let row = PatternString(pattern, padding: generations + 1)
    let storage = [pattern : generations + 1]
    return generate(row, storage: storage)
}

// Recursively generate rows and test until type is found, or limit is reached
func generate(_ row: PatternString, storage: [String : Int]) -> String {
    var newRow = row.nextRow()
    // tests
    if newRow.trimmed.count == 0 {return "vanishing"}
    if storage[newRow.trimmed] == newRow.offset {return "blinking"}
    if storage[newRow.trimmed] != nil {return "gliding"}
    // add new row patterns to storage, and try next row
    var newStorage = storage
    newStorage[newRow.trimmed] = newRow.offset

    if newStorage.count == generations {return "other"}
    return generate(newRow, storage: newStorage)
}

let patterns = try String(contentsOfFile: "patterns.txt", encoding: .utf8).components(separatedBy: .newlines)
for pattern in patterns {print(process(pattern))}
