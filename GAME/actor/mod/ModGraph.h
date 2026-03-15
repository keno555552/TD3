#pragma once

class PartRole {};
class Side {};
class Connector {};
class PartInstance {};
class Connection {};

class ModGraph {
public:
    void addPart(const PartInstance& part);
    void removePart(int partId);
    void connect(int parentId, int childId);
    void disconnect(int parentId, int childId);
    // Additional member functions
};
